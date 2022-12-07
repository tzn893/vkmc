#include "gvk.h"
#include "common.h"

int main()
{
	ptr<gvk::Window> window;
	
	match(gvk::Window::Create(1000, 1000, "vkmc"), w,
		window = w.value(); ,
		return 0;
	);

	while (!window->ShouldClose()) 
	{
		window->UpdateWindow();
	}
}