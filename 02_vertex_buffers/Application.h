#pragma once

#include <Windows.h>
#include <string>

#define DECLARE_APP(WinType)	\
int CALLBACK wWinMain(			\
	HINSTANCE hInst,			\
	HINSTANCE hPrevInst,		\
	LPWSTR lpCmdLine,			\
	int nCmdShow				\
) {								\
	Application<WinType> app(#WinType);	\
	return app.Run();			\
}

template <typename WINDOW>
class Application {
private:
	std::string _name;
	WINDOW* window;
	void Setup()
	{
		window = new WINDOW;
		window->Register();
		window->Create();
		window->ShowWindow(SW_SHOW);
		window->SetText(_name.c_str());
		window->Update();
	}

	void Loop()
	{
		MSG msg;

		ZeroMemory(&msg, sizeof(MSG));
		while (true) {
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT) {
					break;
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else {
				window->drawFrame();
			}
		}
	}

	void Teardown()
	{
		//window->Shutdown();
		delete window;
	}
public:
	Application(const std::string& name)
		: _name(name)
	{

	}

	int Run()
	{
		Setup();
		Loop();
		Teardown();
		return 0;
	}
};
