#pragma once

#include <Windows.h>
#include "Observable.h"

class Window :
	public Observable <UINT, WPARAM, LPARAM>
{
private:
	HWND _window;
private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
protected:
	inline HWND Handle() const { return _window; }
	void ShowLastError() const;
public:
	Window();
	virtual ~Window();

	ATOM Register();
	void Create();
	void Update();
	void ShowWindow(int nCmdShow);
	void Close();
	virtual void Destroy();
	SIZE Size() const;
	inline LONG width() const { return Size().cx; }
	inline LONG height() const { return Size().cy; }
	virtual void Size(LONG cx, LONG cy);
	virtual void Size(SIZE sz);

	virtual LPCTSTR ClassName() { return TEXT("BasicWindow"); }
	virtual LPCTSTR WindowName() { return ClassName(); }
	virtual UINT ClassStyle() { return CS_HREDRAW | CS_VREDRAW; }
	virtual UINT WindowStyle() { return WS_OVERLAPPEDWINDOW; }
	virtual void drawFrame() {}

	void SetText(const char* text) {
		SetWindowTextA(_window, text);
	}

};

