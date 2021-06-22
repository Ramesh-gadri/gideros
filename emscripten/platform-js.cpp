#include <platform.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

std::vector<std::string> getDeviceInfo()
{
  std::vector<std::string> result;
        
  result.push_back("Web");
	char *userAgentC=(char *) EM_ASM_INT_V({
	 return allocate(intArrayFromString(navigator.userAgent), 'i8', ALLOC_STACK);
	});
  std::string userAgent=userAgentC;
  result.push_back(userAgent);
                                
 return result;
}

std::string getAppId(){
	return "";
}

void getSafeDisplayArea(int &x,int &y,int &w,int &h)
{
}

void setKeepAwake(bool awake)
{
}

bool setKeyboardVisibility(bool visible){
	return false;
}

bool setTextInput(int type,const char *buffer,int selstart,int selend,const char *label,const char *actionLabel, const char *hintText)
{
	return false;
}

int setClipboard(std::string data,std::string mimeType, int luaFunc) {
	return -1;
}

int getClipboard(std::string &data,std::string &mimeType, int luaFunc) {
	return -1;
}

extern int s_KeyboardModifiers;
int getKeyboardModifiers() {
	return s_KeyboardModifiers;
}

void vibrate(int ms)
{
	EM_ASM_({
		window.navigator.vibrate($0);
	},ms);
}

void setWindowSize(int width, int height){
	//TODO
}

void setFullScreen(bool fullScreen){
	emscripten_request_fullscreen("canvas",true);
}

std::string getDeviceName(){
	return ""; //TODO
}

std::string getLocale()
{
	return ""; //TODO
}

std::string getLanguage()
{
	char *lang=(char *) EM_ASM_INT_V({
	 return allocate(intArrayFromString(Module.gplatformLanguage()), 'i8', ALLOC_STACK);
	});

	return lang;
}

void openUrl(const char* url)
{
	EM_ASM_({
		window.open(UTF8ToString($0));
	},url);
}

bool canOpenUrl(const char *url)
{
    return true;
}

bool g_checkStringProperty(bool isSet, const char* what)
{
 return false;
}

extern const char *currentUrl;
const char* g_getProperty(const char* what, const char* arg)
{
 if (!strcmp(what,"currentUrl"))
  return currentUrl;
 return NULL;
}

void g_setProperty(const char* what, const char* arg)
{
}

static int s_fps = 60;

extern "C" {

int g_getFps()
{
    return s_fps;
}

void g_setFps(int fps)
{
    s_fps = fps;
}


}


void g_exit()
{
    exit(0);
}
