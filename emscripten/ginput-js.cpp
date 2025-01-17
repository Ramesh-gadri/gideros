#include <ginput.h>
#include <ginput-js.h>
#include <vector>
#include <gevent.h>
#include <memory.h>
#include <string.h>
#include <pthread.h>
#include <string>
#include <map>
#include <ctype.h>
#include <glog.h>

struct Pointer{
	int x;
	int y;
	int id;
};

std::map<unsigned int, Pointer> m_pointerIds;

namespace ginput {

class InputManager
{
public:
    InputManager()
    {
        isMouseToTouchEnabled_ = 0;
        isTouchToMouseEnabled_ = 0;
        mouseTouchOrder_= 0;

        keyMap_["ArrowLeft"] = GINPUT_KEY_LEFT;
        keyMap_["Left"] = GINPUT_KEY_LEFT;
        keyMap_["\x25"] = GINPUT_KEY_LEFT;
        keyMap_["ArrowRight"] = GINPUT_KEY_RIGHT;
        keyMap_["Right"] = GINPUT_KEY_RIGHT;
        keyMap_["\x27"] = GINPUT_KEY_RIGHT;
        keyMap_["ArrowUp"] = GINPUT_KEY_UP;
        keyMap_["Up"] = GINPUT_KEY_UP;
        keyMap_["\x26"] = GINPUT_KEY_UP;
        keyMap_["ArrowDown"] = GINPUT_KEY_DOWN;
        keyMap_["Down"] = GINPUT_KEY_DOWN;
        keyMap_["\x28"] = GINPUT_KEY_DOWN;

		//keyMap_[GINPUT_KEY_BACK] = GINPUT_KEY_BACK;

   	keyMap_["Shift"] = GINPUT_KEY_SHIFT;
   	keyMap_["ShiftLeft"] = GINPUT_KEY_SHIFT;
   	keyMap_["ShiftRight"] = GINPUT_KEY_SHIFT;
	keyMap_["\x10"] = GINPUT_KEY_SHIFT;
	keyMap_["Backspace"] = GINPUT_KEY_BACKSPACE;
	keyMap_["\x08"] = GINPUT_KEY_BACKSPACE;
	keyMap_["Space"] = GINPUT_KEY_SPACE;
	keyMap_["Control"] = GINPUT_KEY_CTRL;

	keyMap_["AltLeft"] = GINPUT_KEY_ALT;
	keyMap_["AltRight"] = GINPUT_KEY_ALT;
	keyMap_["Escape"] = GINPUT_KEY_ESC;
	keyMap_["Tab"] = GINPUT_KEY_TAB;

    keyMap_["Home"] = GINPUT_KEY_HOME;
    keyMap_["End"] = GINPUT_KEY_END;
    keyMap_["Insert"] = GINPUT_KEY_INSERT;
    keyMap_["Delete"] = GINPUT_KEY_DELETE;
    keyMap_["PageUp"] = GINPUT_KEY_PAGEUP;
    keyMap_["PageDown"] = GINPUT_KEY_PAGEDOWN;
    keyMap_["Enter"] = GINPUT_KEY_ENTER;
    keyMap_["NumpadEnter"] = GINPUT_KEY_ENTER;



        pthread_mutex_init(&touchPoolMutex_, NULL);

        gevent_AddCallback(posttick_s, this);

        gid_ = g_NextId();
    }

    ~InputManager()
    {
        gevent_RemoveCallbackWithGid(gid_);

        gevent_RemoveCallback(posttick_s, this);

        for (size_t i = 0; i < mousePool1_.size(); ++i)
            delete mousePool1_[i];
        for (size_t i = 0; i < mousePool2_.size(); ++i)
            delete mousePool2_[i];

        for (size_t i = 0; i < keyPool1_.size(); ++i)
            delete keyPool1_[i];
        for (size_t i = 0; i < keyPool2_.size(); ++i)
            delete keyPool2_[i];


        pthread_mutex_lock(&touchPoolMutex_);
        std::map<size_t, std::vector<ginput_TouchEvent*> >::iterator iter;
        for (iter = touchPool1_.begin(); iter != touchPool1_.end(); ++iter)
        {
            const std::vector<ginput_TouchEvent*> &v = iter->second;
            for (size_t i = 0; i < v.size(); ++i)
            {
                delete [] v[i]->allTouches;
                delete v[i];
            }
        }
        for (iter = touchPool2_.begin(); iter != touchPool2_.end(); ++iter)
        {
            const std::vector<ginput_TouchEvent*> &v = iter->second;
            for (size_t i = 0; i < v.size(); ++i)
            {
                delete [] v[i]->allTouches;
                delete v[i];
            }
        }
        pthread_mutex_unlock(&touchPoolMutex_);

        pthread_mutex_destroy(&touchPoolMutex_);
    }

    static void posttick_s(int type, void *event, void *udata)
    {
        if (type == GEVENT_POST_TICK_EVENT)
            static_cast<InputManager*>(udata)->posttick();
    }

    void posttick()
    {
        for (size_t i = 0; i < mousePool2_.size(); ++i)
            mousePool1_.push_back(mousePool2_[i]);
        mousePool2_.clear();

        for (size_t i = 0; i < keyPool2_.size(); ++i)
            keyPool1_.push_back(keyPool2_[i]);
        keyPool2_.clear();


        pthread_mutex_lock(&touchPoolMutex_);
        std::map<size_t, std::vector<ginput_TouchEvent*> >::iterator iter;
        for (iter = touchPool2_.begin(); iter != touchPool2_.end(); ++iter)
        {
            const std::vector<ginput_TouchEvent*> &v = iter->second;
            for (size_t i = 0; i < v.size(); ++i)
                touchPool1_[iter->first].push_back(v[i]);
        }
        touchPool2_.clear();
        pthread_mutex_unlock(&touchPoolMutex_);
    }

    void keyDown(const char *kval,const char *kcode, int modifiers)
    {
        int keyCode = convertKeyCode(kval);

        ginput_KeyEvent *event = newKeyEvent(keyCode, 0, modifiers); //TODO SCAN CODE
        gevent_EnqueueEvent(gid_, callback_s, GINPUT_KEY_DOWN_EVENT, event, 0, this);
        deleteKeyEvent(event);
    }

    void keyUp(const char *kval,const char *kcode, int modifiers)
    {
        int keyCode = convertKeyCode(kval);

        ginput_KeyEvent *event = newKeyEvent(keyCode, 0, modifiers); //TODO SCAN CODE
        gevent_EnqueueEvent(gid_, callback_s, GINPUT_KEY_UP_EVENT, event, 0, this);
        deleteKeyEvent(event);
    }
    
    void keyChar(const char *keychar)
    {
        ginput_KeyEvent *event = newKeyEvent(0,0,-1);
        if (strlen(keychar)<(sizeof(event->charCode)))
        {
            strcpy(event->charCode,keychar);
            gevent_EnqueueEvent(gid_, callback_s, GINPUT_KEY_CHAR_EVENT, event, 0, this);
        }
        deleteKeyEvent(event);
    }
                                                                               
    void setMouseToTouchEnabled(int enabled)
    {
        isMouseToTouchEnabled_ = enabled;
    }

    void setTouchToMouseEnabled(int enabled)
    {
        isTouchToMouseEnabled_ = enabled;
    }

    void setMouseTouchOrder(int order)
    {
        mouseTouchOrder_ = order;
    }

	void mouseDown(int x, int y, int button,int mod)
	{
		ginput_MouseEvent *mouseEvent = newMouseEvent(x, y, button, mod);

		ginput_TouchEvent *touchEvent = NULL;
		if (isMouseToTouchEnabled_)
		{
			touchEvent = newTouchEvent(1);
			touchEvent->touch.x = x;
			touchEvent->touch.y = y;
			touchEvent->touch.id = 0;
			touchEvent->touch.pressure = 0;
			touchEvent->touch.touchType = 2;
			touchEvent->touch.modifiers = mod;
			touchEvent->touch.mouseButton=button;
			touchEvent->allTouches[0].x = x;
			touchEvent->allTouches[0].y = y;
			touchEvent->allTouches[0].id = 0;
			touchEvent->allTouches[0].pressure = 0;
			touchEvent->allTouches[0].touchType = 2;
			touchEvent->allTouches[0].modifiers = mod;
			touchEvent->allTouches[0].mouseButton=button;
		}

		if (mouseTouchOrder_ == 0)
		{
			gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_DOWN_EVENT, mouseEvent, 0, this);
			deleteMouseEvent(mouseEvent);
			if (touchEvent)
			{
				gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_BEGIN_EVENT, touchEvent, 0, this);
				deleteTouchEvent(touchEvent);
			}

		}
		else
		{
			if (touchEvent)
			{
				gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_BEGIN_EVENT, touchEvent, 0, this);
				deleteTouchEvent(touchEvent);
			}
			gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_DOWN_EVENT, mouseEvent, 0, this);
			deleteMouseEvent(mouseEvent);
		}
	}

	void mouseMove(int x, int y, int button, int mod)
	{
		ginput_MouseEvent *mouseEvent = newMouseEvent(x, y, button, mod);

		ginput_TouchEvent *touchEvent = NULL;
		if (isMouseToTouchEnabled_)
		{
			touchEvent = newTouchEvent(1);
			touchEvent->touch.x = x;
			touchEvent->touch.y = y;
			touchEvent->touch.id = 0;
			touchEvent->touch.pressure = 0;
			touchEvent->touch.touchType = 2;
			touchEvent->touch.modifiers = mod;
			touchEvent->touch.mouseButton=button;
			touchEvent->allTouches[0].x = x;
			touchEvent->allTouches[0].y = y;
			touchEvent->allTouches[0].id = 0;
			touchEvent->allTouches[0].pressure = 0;
			touchEvent->allTouches[0].touchType = 2;
			touchEvent->allTouches[0].modifiers = mod;
			touchEvent->allTouches[0].mouseButton=button;
		}

		if (mouseTouchOrder_ == 0)
		{
			gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_MOVE_EVENT, mouseEvent, 0, this);
			deleteMouseEvent(mouseEvent);
			if (touchEvent)
			{
				gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_MOVE_EVENT, touchEvent, 0, this);
				deleteTouchEvent(touchEvent);
			}

		}
		else
		{
			if (touchEvent)
			{
				gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_MOVE_EVENT, touchEvent, 0, this);
				deleteTouchEvent(touchEvent);
			}
			gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_MOVE_EVENT, mouseEvent, 0, this);
			deleteMouseEvent(mouseEvent);
		}
	}

	void mouseHover(int x, int y, int button, int mod)
	{
		ginput_MouseEvent *mouseEvent = newMouseEvent(x, y, button, mod);

		gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_HOVER_EVENT, mouseEvent, 0, this);
		deleteMouseEvent(mouseEvent);
	}

	void mouseUp(int x, int y, int button, int mod)
	{
		ginput_MouseEvent *mouseEvent = newMouseEvent(x, y, button, mod);

		ginput_TouchEvent *touchEvent = NULL;
		if (isMouseToTouchEnabled_)
		{
			touchEvent = newTouchEvent(1);
			touchEvent->touch.x = x;
			touchEvent->touch.y = y;
			touchEvent->touch.id = 0;
			touchEvent->touch.pressure = 0;
			touchEvent->touch.touchType = 2;
			touchEvent->touch.modifiers = mod;
			touchEvent->touch.mouseButton=button;
			touchEvent->allTouches[0].x = x;
			touchEvent->allTouches[0].y = y;
			touchEvent->allTouches[0].id = 0;
			touchEvent->allTouches[0].pressure = 0;
			touchEvent->allTouches[0].touchType = 2;
			touchEvent->allTouches[0].modifiers = mod;
			touchEvent->allTouches[0].mouseButton=button;
		}

		if (mouseTouchOrder_ == 0)
		{
			gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_UP_EVENT, mouseEvent, 0, this);
			deleteMouseEvent(mouseEvent);
			if (touchEvent)
			{
				gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_END_EVENT, touchEvent, 0, this);
				deleteTouchEvent(touchEvent);
			}

		}
		else
		{
			if (touchEvent)
			{
				gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_END_EVENT, touchEvent, 0, this);
				deleteTouchEvent(touchEvent);
			}
			gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_UP_EVENT, mouseEvent, 0, this);
			deleteMouseEvent(mouseEvent);
		}
	}

    void mouseWheel(int x, int y, int buttons,int delta, int mod)
    {
        ginput_MouseEvent *mouseEvent = newMouseEvent(x, y, buttons, mod);
        mouseEvent->wheel=delta;
        gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_WHEEL_EVENT, mouseEvent, 0, this);
        deleteMouseEvent(mouseEvent);
    }

	void touchBegin(int x, int y, int id, int mod)
	{
		ginput_TouchEvent *touchEvent = newTouchEvent(m_pointerIds.size());

		touchEvent->touch.x = x;
		touchEvent->touch.y = y;
		touchEvent->touch.id = addTouch(id);

		int i = 0;
		for (auto it = m_pointerIds.begin(); it != m_pointerIds.end(); ++it){
			touchEvent->allTouches[i].x = it->second.x;
			touchEvent->allTouches[i].y = it->second.y;
			touchEvent->allTouches[i].pressure = 0;
			touchEvent->allTouches[i].touchType = 0;
			touchEvent->allTouches[i].mouseButton = 0;
			touchEvent->allTouches[i].modifiers = 0;
			touchEvent->allTouches[i].id = addTouch(it->second.id);
			i++;
		}

		ginput_MouseEvent *mouseEvent = NULL;
		if (isTouchToMouseEnabled_ && touchEvent->touch.id == 0) {
			mouseEvent = newMouseEvent(touchEvent->touch.x, touchEvent->touch.y, GINPUT_LEFT_BUTTON, mod);
			mouseEvent->mouseType = 2; //Finger (assumed)
		}

		if (mouseTouchOrder_ == 0)
		{
			if (mouseEvent)
			{
				gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_DOWN_EVENT, mouseEvent, 0, this);
				deleteMouseEvent(mouseEvent);
			}
			gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_BEGIN_EVENT, touchEvent, 0, this);
			deleteTouchEvent(touchEvent);
		}
		else
		{
			gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_BEGIN_EVENT, touchEvent, 0, this);
			deleteTouchEvent(touchEvent);
			if (mouseEvent)
			{
				gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_DOWN_EVENT, mouseEvent, 0, this);
				deleteMouseEvent(mouseEvent);
			}
		}
	}

	void touchMove(int x, int y, int id, int mod)
	{
		ginput_TouchEvent *touchEvent = newTouchEvent(m_pointerIds.size());

		touchEvent->touch.x = x;
		touchEvent->touch.y = y;
		touchEvent->touch.id = addTouch(id);
		touchEvent->touch.modifiers = mod;

		int i = 0;
		for (auto it = m_pointerIds.begin(); it != m_pointerIds.end(); ++it){
			touchEvent->allTouches[i].x = it->second.x;
			touchEvent->allTouches[i].y = it->second.y;
			touchEvent->allTouches[i].pressure = 0;
			touchEvent->allTouches[i].touchType = 0;
			touchEvent->allTouches[i].mouseButton = 0;
			touchEvent->allTouches[i].modifiers = 0;
			touchEvent->allTouches[i].id = addTouch(it->second.id);
			i++;
		}

		ginput_MouseEvent *mouseEvent = NULL;
		if (isTouchToMouseEnabled_ && touchEvent->touch.id == 0) {
			mouseEvent = newMouseEvent(touchEvent->touch.x, touchEvent->touch.y, GINPUT_NO_BUTTON,mod);
			mouseEvent->mouseType = 2; //Finger (assumed)
		}

		if (mouseTouchOrder_ == 0)
		{
			if (mouseEvent)
			{
				gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_MOVE_EVENT, mouseEvent, 0, this);
				deleteMouseEvent(mouseEvent);
			}
			gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_MOVE_EVENT, touchEvent, 0, this);
			deleteTouchEvent(touchEvent);
		}
		else
		{
			gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_MOVE_EVENT, touchEvent, 0, this);
			deleteTouchEvent(touchEvent);
			if (mouseEvent)
			{
				gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_MOVE_EVENT, mouseEvent, 0, this);
				deleteMouseEvent(mouseEvent);
			}
		}
	}

	void touchEnd(int x, int y, int id, int mod)
	{
		ginput_TouchEvent *touchEvent = newTouchEvent(m_pointerIds.size());

		touchEvent->touch.x = x;
		touchEvent->touch.y = y;
		touchEvent->touch.id = addTouch(id);
		touchEvent->touch.modifiers = mod;

		int i = 0;
		for (auto it = m_pointerIds.begin(); it != m_pointerIds.end(); ++it){
			touchEvent->allTouches[i].x = it->second.x;
			touchEvent->allTouches[i].y = it->second.y;
			touchEvent->allTouches[i].pressure = 0;
			touchEvent->allTouches[i].touchType = 0;
			touchEvent->allTouches[i].mouseButton = 0;
			touchEvent->allTouches[i].modifiers = 0;
			touchEvent->allTouches[i].id = addTouch(it->second.id);
			i++;
		}

		ginput_MouseEvent *mouseEvent = NULL;
		if (isTouchToMouseEnabled_ && touchEvent->touch.id == 0) {
			mouseEvent = newMouseEvent(touchEvent->touch.x, touchEvent->touch.y, GINPUT_LEFT_BUTTON,mod);
			mouseEvent->mouseType = 2; //Finger (assumed)
		}

		if (mouseTouchOrder_ == 0)
		{
			if (mouseEvent)
			{
				gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_UP_EVENT, mouseEvent, 0, this);
				deleteMouseEvent(mouseEvent);
			}
			gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_END_EVENT, touchEvent, 0, this);
			deleteTouchEvent(touchEvent);
		}
		else
		{
			gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_END_EVENT, touchEvent, 0, this);
			deleteTouchEvent(touchEvent);
			if (mouseEvent)
			{
				gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_UP_EVENT, mouseEvent, 0, this);
				deleteMouseEvent(mouseEvent);
			}
		}
		removeTouch(id);
	}

	void touchCancel(int x, int y, int id, int mod)
	{
		ginput_TouchEvent *touchEvent = newTouchEvent(m_pointerIds.size());

		touchEvent->touch.x = x;
		touchEvent->touch.y = y;
		touchEvent->touch.id = addTouch(id);
		touchEvent->touch.modifiers = mod;

		int i = 0;
		for (auto it = m_pointerIds.begin(); it != m_pointerIds.end(); ++it){
			touchEvent->allTouches[i].x = it->second.x;
			touchEvent->allTouches[i].y = it->second.y;
			touchEvent->allTouches[i].pressure = 0;
			touchEvent->allTouches[i].touchType = 0;
			touchEvent->allTouches[i].mouseButton = 0;
			touchEvent->allTouches[i].modifiers = 0;
			touchEvent->allTouches[i].id = addTouch(it->second.id);
			i++;
		}

		ginput_MouseEvent *mouseEvent = NULL;
		if (isTouchToMouseEnabled_ && touchEvent->touch.id == 0) {
			mouseEvent = newMouseEvent(touchEvent->touch.x, touchEvent->touch.y, GINPUT_LEFT_BUTTON,mod);
			mouseEvent->mouseType = 2; //Finger (assumed)
		}

		if (mouseTouchOrder_ == 0)
		{
			if (mouseEvent)
			{
				gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_UP_EVENT, mouseEvent, 0, this);
				deleteMouseEvent(mouseEvent);
			}
			gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_CANCEL_EVENT, touchEvent, 0, this);
			deleteTouchEvent(touchEvent);
		}
		else
		{
			gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_CANCEL_EVENT, touchEvent, 0, this);
			deleteTouchEvent(touchEvent);
			if (mouseEvent)
			{
				gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_UP_EVENT, mouseEvent, 0, this);
				deleteMouseEvent(mouseEvent);
			}
		}
		removeTouch(id);
	}

private:
	int addTouch(int touch)
	{
		for (int i = 0; i < touches_.size(); ++i)
			if (touches_[i] == touch)
				return i;

		for (int i = 0; i < touches_.size(); ++i)
			if (touches_[i] == NULL)
			{
				touches_[i] = touch;
				return i;
			}

		touches_.push_back(touch);

		return touches_.size() - 1;
	}

	void removeTouch(int touch)
	{
		for (int i = 0; i < touches_.size(); ++i)
			if (touches_[i] == touch)
			{
				touches_[i] = NULL;
				break;
			}
	}

	std::vector<int> touches_;

    ginput_MouseEvent *newMouseEvent(int x, int y, int button,int mod)
    {
        ginput_MouseEvent *event;

        if (mousePool1_.empty())
        {
            event = new ginput_MouseEvent;
        }
        else
        {
            event = mousePool1_.back();
            mousePool1_.pop_back();
        }

        event->x = x;
        event->y = y;
        event->button = button;
        event->wheel=0;
        event->modifiers=mod;
        event->mouseType=0;

        return event;
    }

    void deleteMouseEvent(ginput_MouseEvent *event)
    {
        mousePool2_.push_back(event);
    }

    ginput_KeyEvent *newKeyEvent(int keyCode, int realCode, int modifiers)
    {
        ginput_KeyEvent *event;

        if (keyPool1_.empty())
        {
            event = new ginput_KeyEvent;
        }
        else
        {
            event = keyPool1_.back();
            keyPool1_.pop_back();
        }

        event->keyCode = keyCode;
		event->realCode = realCode;
		event->modifiers = modifiers;

        return event;
    }

    void deleteKeyEvent(ginput_KeyEvent *event)
    {
        keyPool2_.push_back(event);
    }

    int convertKeyCode(const char *kval)
    {
        std::string kv=kval;
        std::map<std::string, int>::const_iterator iter = keyMap_.find(kv);

        if (iter == keyMap_.end())
        {
            if (strlen(kval)>1)
                glog_i("Key code '%s' not handled!",kval);
            return toupper(*kval); //Default to first char of keyVal
        }

        return iter->second;
    }

    ginput_TouchEvent *newTouchEvent(size_t allTouchesCount)
    {
        pthread_mutex_lock(&touchPoolMutex_);
        std::vector<ginput_TouchEvent*> &pool = touchPool1_[allTouchesCount];

        ginput_TouchEvent *event;

        if (pool.empty())
        {
            event = new ginput_TouchEvent;
            event->allTouches = new ginput_Touch[allTouchesCount];
        }
        else
        {
            event = pool.back();
            pool.pop_back();
        }
        pthread_mutex_unlock(&touchPoolMutex_);

        event->allTouchesCount = allTouchesCount;
        event->touch.pressure=0;
        event->touch.touchType=0;
        event->touch.mouseButton=0;
        event->touch.modifiers=0;

        return event;
    }

    void deleteTouchEvent(ginput_TouchEvent *event)
    {
        pthread_mutex_lock(&touchPoolMutex_);
        touchPool2_[event->allTouchesCount].push_back(event);
        pthread_mutex_unlock(&touchPoolMutex_);
    }

private:
    std::vector<ginput_MouseEvent*> mousePool1_;
    std::vector<ginput_MouseEvent*> mousePool2_;
    std::vector<ginput_KeyEvent*> keyPool1_;
    std::vector<ginput_KeyEvent*> keyPool2_;
    std::map<size_t, std::vector<ginput_TouchEvent*> > touchPool1_;
    std::map<size_t, std::vector<ginput_TouchEvent*> > touchPool2_;

    std::map<std::string, int> keyMap_;

    pthread_mutex_t touchPoolMutex_;

    int isMouseToTouchEnabled_;
    int isTouchToMouseEnabled_;
    int mouseTouchOrder_;

public:
    g_id addCallback(gevent_Callback callback, void *udata)
    {
        return callbackList_.addCallback(callback, udata);
    }

    void removeCallback(gevent_Callback callback, void *udata)
    {
        callbackList_.removeCallback(callback, udata);
    }

    void removeCallbackWithGid(g_id gid)
    {
        callbackList_.removeCallbackWithGid(gid);
    }

    static void callback_s(int type, void *event, void *udata)
    {
        static_cast<InputManager*>(udata)->callbackList_.dispatchEvent(type, event);
    }

private:
    gevent_CallbackList callbackList_;
    g_id gid_;
};

}

using namespace ginput;

static InputManager *s_manager = NULL;

extern "C" {

void ginput_init()
{
    s_manager = new InputManager;
}

void ginput_cleanup()
{
    delete s_manager;
    s_manager = NULL;
}

int ginput_isAccelerometerAvailable()
{
    return 0;
}

void ginput_startAccelerometer()
{

}

void ginput_stopAccelerometer()
{

}

void ginput_getAcceleration(double *x, double *y, double *z)
{
    if (x)
        *x = 0;
    if (y)
        *y = 0;
    if (z)
        *z = 0;
}

int ginput_isGyroscopeAvailable()
{
    return 0;
}

void ginput_startGyroscope()
{

}

void ginput_stopGyroscope()
{

}

void ginput_getGyroscopeRotationRate(double *x, double *y, double *z)
{
    if (x)
        *x = 0;
    if (y)
        *y = 0;
    if (z)
        *z = 0;
}

void ginputp_keyDown(const char *keyVal,const char *keyCode,int modifiers)
{
    if (s_manager)
        s_manager->keyDown(keyVal,keyCode, modifiers);
}

void ginputp_keyUp(const char *keyVal,const char *keyCode,int modifiers)
{
    if (s_manager)
        s_manager->keyUp(keyVal,keyCode, modifiers);
}

void ginputp_keyChar(const char *keyChar)
{
    if (s_manager)
        s_manager->keyChar(keyChar);
}            

void ginput_setMouseToTouchEnabled(int enabled)
{
    s_manager->setMouseToTouchEnabled(enabled);
}

void ginput_setTouchToMouseEnabled(int enabled)
{
    s_manager->setTouchToMouseEnabled(enabled);
}

void ginput_setMouseTouchOrder(int order)
{
    s_manager->setMouseTouchOrder(order);
}

g_id ginput_addCallback(gevent_Callback callback, void *udata)
{
    return s_manager->addCallback(callback, udata);
}

void ginput_removeCallback(gevent_Callback callback, void *udata)
{
    s_manager->removeCallback(callback, udata);
}

void ginput_removeCallbackWithGid(g_id gid)
{
    s_manager->removeCallbackWithGid(gid);
}

void ginputp_mouseDown(int x, int y, int button, int mod)
{
	if (s_manager)
		s_manager->mouseDown(x, y, button, mod);
}

void ginputp_mouseMove(int x, int y, int button, int mod)
{
	if (s_manager)
		s_manager->mouseMove(x, y, button, mod);
}

void ginputp_mouseHover(int x, int y, int button, int mod)
{
	if (s_manager)
		s_manager->mouseHover(x, y, button, mod);
}

void ginputp_mouseUp(int x, int y, int button, int mod)
{
	if (s_manager)
		s_manager->mouseUp(x, y, button, mod);
}

void ginputp_mouseWheel(int x, int y, int buttons, int delta, int mod)
{
    if (s_manager)
        s_manager->mouseWheel(x, y, buttons,delta, mod);
}

void ginputp_touchBegin(int x, int y, int id, int mod){
	if (s_manager){
		Pointer pointer;
		pointer.x = x;
		pointer.y = y;
		pointer.id = id;
		m_pointerIds[id]= pointer;
		s_manager->touchBegin(x, y, id, mod);
	}
}

void ginputp_touchMove(int x, int y, int id, int mod){
	Pointer* p = &m_pointerIds[id];
	if (s_manager && (x != p->x || y != p->y)){
		p->x = x;
		p->y = y;
		s_manager->touchMove(x, y, id, mod);
	}
}

void ginputp_touchEnd(int x, int y, int id, int mod){
	if (s_manager){
		s_manager->touchEnd(x, y, id, mod);
		m_pointerIds.erase(id);
	}
}

void ginputp_touchCancel(int x, int y, int id, int mod){
	if (s_manager){
		s_manager->touchCancel(x, y, id, mod);
		m_pointerIds.erase(id);
	}
}

}
