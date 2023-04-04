#include "gvk.h"
#include "common.h"

#include "renderer/renderer.h"
#include "terrian/terrian.h"
#include "util/timer.h"


Vector3f cameraPos = Vector3f(0.0f, 126.0f, -30.0f);
Vector3f cameraFront = Vector3f(0.0f, 0.0f, 1.0f);
Vector3f cameraUp = Vector3f(0.0f, 1.0f, 0.0f);

float cameraSpeed = 5.0f;

bool firstMouse = true;
float yaw =   0.0f;// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float fov = 45.0f;

float sensitive = 30.0f;

float cameraRadius = 120.f;

int main()
{
	ptr<gvk::Window> window;
	
	match
	(
		gvk::Window::Create(1000, 1000, "vkmc"), w,
		window = w.value(); ,
		return 0;
	);

	ptr<Renderer>	renderer = std::make_shared<Renderer>(window);

	u32 seed = 114514;

	std::string terrian_atlas_path = std::string(VKMC_ROOT_DIRECTORY) + "/asset/blocks-test.png";
	std::string terrian_file_path = std::string(VKMC_ROOT_DIRECTORY) + "/asset/save-" + std::to_string(seed) + ".sav";
	ptr<Terrian> terrian = std::make_shared<Terrian>(terrian_atlas_path, terrian_file_path, seed);//(new Terrian(terrian_path, 114514));

	CubeMapDesc desc;
	desc.front = std::string(VKMC_ROOT_DIRECTORY) + "/asset/Daylight Box_Front.png";
	desc.back = std::string(VKMC_ROOT_DIRECTORY) + "/asset/Daylight Box_Back.png" ;
	desc.left = std::string(VKMC_ROOT_DIRECTORY) + "/asset/Daylight Box_Left.png" ;
	desc.right = std::string(VKMC_ROOT_DIRECTORY) +"/asset/Daylight Box_Right.png";
	desc.up = std::string(VKMC_ROOT_DIRECTORY) + "/asset/Daylight Box_Top.png"   ;
	desc.down = std::string(VKMC_ROOT_DIRECTORY) + "/asset/Daylight Box_Bottom.png" ;

	ptr<Skybox>	 skybox = std::make_shared<Skybox>(desc);

	MainCamera& camera = Singleton<MainCamera>().Get();
	Timer& timer = Singleton<Timer>().Get();

	camera.Initialize(1000, 1000);
	camera.SetFar(cameraRadius);

	camera.SetPosition(cameraPos);
	camera.SetFront(Vector3f( Math::radians(yaw),Math::radians(pitch), 0));

	if (!renderer->Initialize() || !terrian->Initialize())
	{
		return -1;
	}

	if (!renderer->AddTerrian(terrian))
	{
		return -1;
	}

	if (!renderer->AddSkybox(skybox))
	{
		return -1;
	}

	while (!window->ShouldClose()) 
	{
		timer.Tick();

		terrian->Update(window);

		float delta_time = timer.DeltaTime();

		float speed = cameraSpeed;
		if (window->KeyHold(GVK_KEY_SHIFT))
		{
			speed *= 5.f;
		}

		//update main camera
		if (window->KeyHold(GVK_KEY_W))
		{
			cameraPos += speed * delta_time * cameraFront;
		}
		if (window->KeyHold(GVK_KEY_S))
		{
			cameraPos -= speed * delta_time * cameraFront;
		}
		if (window->KeyHold(GVK_KEY_A))
		{
			cameraPos += Math::normalize(glm::cross(cameraFront, cameraUp)) * speed * delta_time;
		}
		if (window->KeyHold(GVK_KEY_D))
		{
			cameraPos -= Math::normalize(glm::cross(cameraFront, cameraUp)) * speed * delta_time;
		}

		if (window->KeyHold(GVK_MOUSE_1) && window->MouseMove())
		{
			GvkVector2 offset = window->GetMouseOffset();
			yaw += offset.x * sensitive * delta_time;
			pitch -=  offset.y * sensitive * delta_time;
			if (pitch > 89.99f) pitch = 89.98f;
			if (pitch < -89.99f) pitch = -89.98f;

			Vector3f front;
			front.z = cos(Math::radians(yaw)) * cos(Math::radians(pitch));
			front.y = sin(Math::radians(pitch));
			front.x = sin(Math::radians(yaw)) * cos(Math::radians(pitch));
			cameraFront = Math::normalize(front);
		}

		camera.SetPosition(cameraPos);
		camera.SetFront(cameraFront);

		renderer->Tick();

		window->UpdateWindow();
	}
}