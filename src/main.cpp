
#include "core/Application.h"
#include <memory>

int main()
{
	auto app = std::make_unique<mi_3d::Application>();
	if (!app->Initialize(640, 480, "MI_3D"))
		return -1;
	app->Run();
	return 0;
}