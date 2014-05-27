#include "../InputMonitor.h"
#include "../GameSurface.h"
#include "../InputDevice.h"
#include <GLFW/glfw3.h>
#include <Springbok/Types/Vec3.h>
#include <Springbok/Utils/Debug.h>

namespace
{
#include "../Common/KeyNames.h"
};

#include "WindowUserData.h"

struct GLFWMouse : public InputDevice
{
	GLFWwindow* mWindow;
	GameSurface* mSurface;
	Vec2F mScrollWheel;
	
	virtual const char* getIdentifier() const { return "Mouse"; };
	virtual bool isMouse() const { return true; };
	
	virtual int numberOfButtons() const { return GLFW_MOUSE_BUTTON_LAST; };
	virtual int numberOfCursors() const { return 1; };
	
	virtual bool getButtonState(int index = 0) const 
	{
		return glfwGetMouseButton(mWindow, index);
	};
	virtual bool anyButtonPressed() const
	{
		for (int i = 0; i < GLFW_MOUSE_BUTTON_LAST; ++i)
			if(glfwGetMouseButton(mWindow, i))
				return true;
		return false;
	};
	virtual Vec2F getCursorPosition(int index = 0) const 
	{
		double x, y;
		glfwGetCursorPos(mWindow, &x, &y);
		return Vec2F(x, y) + mSurface->topLeft();
	};
	virtual Vec2F getScrollWheelState(int index = 0) const
	{
		return mScrollWheel;
	};
	virtual void setScrollWheelState(Vec2F value, int index = 0)
	{
		mScrollWheel = value;
	};
};

struct GLFWKeyboard : public InputDevice
{
	GLFWwindow* mWindow;
	
	virtual const char* getIdentifier() const { return "Keyboard"; };
	virtual bool isKeyboard() const { return true; };
		
	virtual int numberOfButtons() const { return GLFW_KEY_LAST; };
	virtual bool getButtonState(int index) const 
	{
		return glfwGetKey(mWindow, index);
	};
	virtual bool anyButtonPressed() const
	{
		for (int i = 1; i < 100; ++i)
			if(glfwGetKey(mWindow, i))
				return true;
		return false;
	};
	virtual std::string getButtonName(int index) const
	{
		return KeyCodeToKeyName(index);
	};
};

struct InputMonitorData
{
	GLFWwindow* Window;
	GameSurface* Surface;
	GLFWKeyboard KeyboardDevice;
	GLFWMouse MouseDevice;
	std::vector<InputDevice*> Devices;
	std::vector<ButtonPressEvent> Events;
};

namespace
{
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if(action == GLFW_REPEAT || action == GLFW_RELEASE)
		return;
	InputMonitorData* d = ((WindowUserData*)glfwGetWindowUserPointer(window))->imd;
	ButtonPressEvent event;
	event.Type = Keyboard;
	event.Button = key;
	event.From = &(d->KeyboardDevice);
	event.Released = action;
	d->Events.push_back(event);
};

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	InputMonitorData* d = ((WindowUserData*)glfwGetWindowUserPointer(window))->imd;
	ButtonPressEvent event;
	event.Type = Mouse;
	event.Button = button;
	event.From = &(d->MouseDevice);
	event.Released = action;
	d->Events.push_back(event);
};

void scrollCallback(GLFWwindow* window, double scrollX, double scrollY)
{
	InputMonitorData* d = ((WindowUserData*)glfwGetWindowUserPointer(window))->imd;
	d->MouseDevice.mScrollWheel[0] += scrollX;
	d->MouseDevice.mScrollWheel[1] += scrollY;
};

}

InputMonitor::InputMonitor(GameSurface* surface)
{
	d = new InputMonitorData;
	GLFWwindow* window = (GLFWwindow*)surface->getWindowHandle();
	d->Surface = surface;
	d->Window = window;
	d->Devices = {&(d->KeyboardDevice), &(d->MouseDevice)};
	d->KeyboardDevice.mWindow = window;
	d->MouseDevice.mWindow = window;
	d->MouseDevice.mSurface = surface;
	glfwSetKeyCallback(window, &keyCallback);
	glfwSetMouseButtonCallback(window, &mouseButtonCallback);
	glfwSetScrollCallback(window, &scrollCallback);
	WindowUserData* userData = (WindowUserData*)glfwGetWindowUserPointer(window);
	userData->imd = d;
};

std::vector< InputDevice* > InputMonitor::getDevices() const
{
	return d->Devices;
}

InputDevice* InputMonitor::getPrimaryPointerDevice() const
{
	return &(d->MouseDevice);
}

InputMonitor::~InputMonitor()
{
	delete d;
};

std::vector<ButtonPressEvent> InputMonitor::poll()
{
	glfwPollEvents();
	{
		std::vector<ButtonPressEvent> retVal;
		retVal.swap(d->Events);
		return retVal;
	}
};