import numpy as np
import pydicom
from pydicom.dataset import Dataset
from pydicom.uid import generate_uid
import os

output_dir = "data/test_ct"
os.makedirs(output_dir, exist_ok=True)

num_slices = 100
rows = 256
cols = 256

series_uid = generate_uid()
study_uid = generate_uid()

for i in range(num_slices):
    filename = os.path.join(output_dir, f"slice_{i:04d}.dcm")

    ds = Dataset()
    ds.file_meta = pydicom.dataset.FileMetaDataset()
    ds.file_meta.MediaStorageSOPClassUID = "1.2.840.10008.5.1.4.1.1.2"
    ds.file_meta.MediaStorageSOPInstanceUID = generate_uid()
    ds.file_meta.TransferSyntaxUID = "1.2.840.10008.1.2.1"

    ds.PatientName = "CHEST^SYNTHETIC"
    ds.PatientID = "CHEST001"
    ds.StudyInstanceUID = study_uid
    ds.SeriesInstanceUID = series_uid
    ds.SOPInstanceUID = ds.file_meta.MediaStorageSOPInstanceUID
    ds.SOPClassUID = ds.file_meta.MediaStorageSOPClassUID
    ds.Modality = "CT"
    ds.InstanceNumber = i + 1

    ds.Rows = rows
    ds.Columns = cols
    ds.BitsAllocated = 16
    ds.BitsStored = 16
    ds.HighBit = 15
    ds.PixelRepresentation = 1
    ds.SamplesPerPixel = 1
    ds.PhotometricInterpretation = "MONOCHROME2"
    ds.PixelSpacing = [0.5, 0.5]
    ds.SliceThickness = 2.0
    ds.ImagePositionPatient = [0.0, 0.0, i * 2.0]
    ds.RescaleIntercept = -1024.0
    ds.RescaleSlope = 1.0

    pixel_data = np.full((rows, cols), -1000, dtype=np.int16)

    cx, cy = cols // 2, rows // 2
    z_frac = i / num_slices

    # body size varies along z (wider at middle, narrower at top/bottom)
    body_rx = int(90 + 20 * np.sin(z_frac * np.pi))
    body_ry = int(70 + 15 * np.sin(z_frac * np.pi))

    for y in range(rows):
        for x in range(cols):
            dx = x - cx
            dy = y - cy

            # body outline (ellipse)
            if (dx / body_rx) ** 2 + (dy / body_ry) ** 2 < 1.0:
                pixel_data[y, x] = 40  # soft tissue

            # skin layer (outer ring of body)
            body_dist = (dx / body_rx) ** 2 + (dy / body_ry) ** 2
            if 0.92 < body_dist < 1.0:
                pixel_data[y, x] = 100  # skin/fat

            # left lung (ellipse, offset left)
            lung_lx = cx - 35
            lung_ly = cy - 5
            lung_rx, lung_ry = 30 + int(10 * np.sin(z_frac * np.pi)), 45 + int(10 * np.sin(z_frac * np.pi))
            if ((x - lung_lx) / lung_rx) ** 2 + ((y - lung_ly) / lung_ry) ** 2 < 1.0:
                pixel_data[y, x] = -800  # air in lung

            # right lung (ellipse, offset right)
            lung_rx2 = cx + 35
            lung_ry2 = cy - 5
            if ((x - lung_rx2) / lung_rx) ** 2 + ((y - lung_ry2) / lung_ry) ** 2 < 1.0:
                pixel_data[y, x] = -800  # air in lung

            # spine (vertical cylinder, posterior)
            spine_cx, spine_cy = cx, cy + 45
            spine_r = 12
            if (x - spine_cx) ** 2 + (y - spine_cy) ** 2 < spine_r ** 2:
                pixel_data[y, x] = 1000  # dense bone

            # spinal canal (small hole in spine)
            canal_r = 4
            if (x - spine_cx) ** 2 + (y - spine_cy) ** 2 < canal_r ** 2:
                pixel_data[y, x] = 20  # spinal fluid

            # ribs (12 pairs, appear at specific z-ranges)
            num_ribs = 12
            for rib in range(num_ribs):
                rib_z_start = rib / num_ribs
                rib_z_end = rib_z_start + 0.04
                if rib_z_start < z_frac < rib_z_end:
                    # left rib
                    rib_dist_l = np.sqrt((x - (cx - 55)) ** 2 + (y - cy) ** 2)
                    if 48 < rib_dist_l < 55 and dx < 0:
                        pixel_data[y, x] = 900  # rib bone

                    # right rib
                    rib_dist_r = np.sqrt((x - (cx + 55)) ** 2 + (y - cy) ** 2)
                    if 48 < rib_dist_r < 55 and dx > 0:
                        pixel_data[y, x] = 900  # rib bone

            # sternum (front, bone)
            if abs(x - cx) < 6 and cy - 60 < y < cy - 45:
                pixel_data[y, x] = 800  # sternum bone

            # heart (appears in middle slices)
            if 0.3 < z_frac < 0.7:
                heart_cx = cx + 10
                heart_cy = cy - 10
                heart_r = int(20 + 10 * np.sin((z_frac - 0.3) / 0.4 * np.pi))
                if (x - heart_cx) ** 2 + (y - heart_cy) ** 2 < heart_r ** 2:
                    pixel_data[y, x] = 50  # heart muscle

            # aorta (descending, next to spine)
            aorta_cx = cx + 8
            aorta_cy = cy + 30
            if (x - aorta_cx) ** 2 + (y - aorta_cy) ** 2 < 10 ** 2:
                pixel_data[y, x] = 45  # blood

    ds.PixelData = pixel_data.tobytes()
    pydicom.dcmwrite(filename, ds)

    if i % 10 == 0:
        print(f"Generated slice {i+1}/{num_slices}")

print(f"Done. {num_slices} slices in {output_dir}/")
print(f"Each slice: {rows}x{cols}, 16-bit signed")
print(f"Anatomy: body outline, lungs, spine, ribs, heart, aorta, sternum")
print(f"HU values: air=-1000, lung=-800, tissue=40, bone=900-1000")