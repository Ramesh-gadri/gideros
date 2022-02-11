#include "textfieldbinder.h"
#include "textfield.h"
#include "tttextfield.h"
#include "font.h"
#include "ttfont.h"
#include "stackchecker.h"
#include "luaapplication.h"
#include "application.h"
#include <luautil.h>


TextFieldBinder::TextFieldBinder(lua_State* L)
{
	Binder binder(L);

	static const luaL_Reg functionList[] = {
        {"setFont", setFont},
		{"getText", getText},
		{"setText", setText},
		{"getTextColor", getTextColor},
		{"setTextColor", setTextColor},
		{"getLetterSpacing", getLetterSpacing},
		{"setLetterSpacing", setLetterSpacing},
        {"getLineHeight", getLineHeight},
        {"getSample", getSample},
        {"setSample", setSample},
        {"getLayout", getLayout},
        {"setLayout", setLayout},
		{"getTextPositionFromPoint", getTextPositionFromPoint},
		{"getPointFromTextPosition", getPointFromTextPosition},
        {NULL, NULL},
	};

	binder.createClass("TextField", "Sprite", create, destruct, functionList);
}

static void populateLayout(lua_State* L,int index,FontBase::TextLayoutParameters *tp)
{
    lua_getfield(L,index,"w"); tp->w=luaL_optnumber(L,-1,0); lua_pop(L,1);
    lua_getfield(L,index,"h"); tp->h=luaL_optnumber(L,-1,0); lua_pop(L,1);
    lua_getfield(L,index,"alignX"); tp->alignx=luaL_optnumber(L,-1,0); lua_pop(L,1);
    lua_getfield(L,index,"alignY"); tp->aligny=luaL_optnumber(L,-1,0); lua_pop(L,1);
    lua_getfield(L,index,"aspect"); tp->aspect=luaL_optnumber(L,-1,100000); lua_pop(L,1);
    lua_getfield(L,index,"flags"); tp->flags=luaL_optinteger(L,-1,(int)FontBase::TLF_NOWRAP); lua_pop(L,1);
    lua_getfield(L,index,"letterSpacing");  tp->letterSpacing=luaL_optnumber(L,-1,0); lua_pop(L,1);
    lua_getfield(L,index,"lineSpacing"); tp->lineSpacing=luaL_optnumber(L,-1,0); lua_pop(L,1);
    lua_getfield(L,index,"tabSpace"); tp->tabSpace=luaL_optnumber(L,-1,4); lua_pop(L,1);
    lua_getfield(L,index,"breakChar"); tp->breakchar=luaL_optstring(L,-1,""); lua_pop(L,1);
}

int TextFieldBinder::create(lua_State* L)
{
	StackChecker checker(L, "TextFieldBinder::create", 1);

	LuaApplication* application = static_cast<LuaApplication*>(luaL_getdata(L));

	Binder binder(L);
	TextFieldBase* textField = NULL;

	FontBase* font = NULL;
	if (lua_isnoneornil(L, 1))
		font = application->getApplication()->getDefaultFont();
	else
		font = static_cast<FontBase*>(binder.getInstance("FontBase", 1));

	const char* str2 = lua_tostring(L, 2);
    const char* str3=NULL;
    FontBase::TextLayoutParameters *layout=NULL,tp;
    if (lua_istable(L,3))
    {
        populateLayout(L,3,&tp);
        layout=&tp;
    }
    else {
        str3 = lua_tostring(L, 3);
        if (lua_istable(L,4))
        {
            populateLayout(L,4,&tp);
            layout=&tp;
        }
    }

    switch (font->getType())
    {
        case FontBase::eFont:
        case FontBase::eTTBMFont:
        case FontBase::eCompositeFont:
            textField = new TextField(application->getApplication(), static_cast<BMFontBase*>(font), str2, str3, layout);
            break;
        case FontBase::eTTFont:
            textField = new TTTextField(application->getApplication(), static_cast<TTFont*>(font), str2, str3, layout);
            break;
    }

/*
	if (lua_gettop(L) == 0)
	{
		textField = new TextField;
	}
	else if (lua_gettop(L) == 1)
	{
		Font* font = static_cast<Font*>(binder.getInstance("Font", 1));
		textField = new TextField(font);
	}
	else
	{
		Font* font = static_cast<Font*>(binder.getInstance("Font", 1));
		const char* text = luaL_checkstring(L, 2);
		textField = new TextField(font, text);
	}
*/

	binder.pushInstance("TextField", textField);

/*	if  (lua_gettop(L) == 1)
	{
		lua_pushvalue(L, 1);
		lua_setfield(L, -2, "__font");
	}
	else
	{
		lua_pushvalue(L, 1);
		lua_setfield(L, -2, "__font");

		lua_pushvalue(L, 2);
		lua_setfield(L, -2, "__text");
	} */

	return 1;
}

int TextFieldBinder::destruct(void *p)
{
	void *ptr = GIDEROS_DTOR_UDATA(p);
	TextFieldBase* textField = static_cast<TextFieldBase*>(ptr);
	textField->unref();

	return 0;
}


int TextFieldBinder::setFont(lua_State* L)
{
    StackChecker checker(L, "TextFieldBinder::setFont", 0);

    Binder binder(L);
    TextFieldBase* textField = static_cast<TextFieldBase*>(binder.getInstance("TextField", 1));
    FontBase* font = static_cast<FontBase*>(binder.getInstance("FontBase", 2));

    textField->setFont(font);

    return 0;
}


int TextFieldBinder::getText(lua_State* L)
{
	StackChecker checker(L, "TextFieldBinder::getText", 1);

	Binder binder(L);
	TextFieldBase* textField = static_cast<TextFieldBase*>(binder.getInstance("TextField", 1));

	lua_pushstring(L, textField->text());

	return 1;
}

int TextFieldBinder::setText(lua_State* L)
{
	StackChecker checker(L, "TextFieldBinder::setText", 0);

	Binder binder(L);
	TextFieldBase* textField = static_cast<TextFieldBase*>(binder.getInstance("TextField", 1));

	const char* text = luaL_checkstring(L, 2);
	textField->setText(text);

	return 0;
}

int TextFieldBinder::getTextColor(lua_State* L)
{
	StackChecker checker(L, "TextFieldBinder::getTextColor", 1);

	Binder binder(L);
	TextFieldBase* textField = static_cast<TextFieldBase*>(binder.getInstance("TextField", 1));
    float r,g,b,a;
    textField->textColor(r,g,b,a);
    unsigned long col=((unsigned long)(r*0xFF0000)&0xFF0000)|((unsigned long)(g*0xFF00)&0xFF00)|((unsigned long)(b*0xFF)&0xFF);

    lua_pushinteger(L, col);
    lua_pushnumber(L, a);

	return 2;
}

int TextFieldBinder::setTextColor(lua_State* L)
{
	StackChecker checker(L, "TextFieldBinder::setTextColor", 0);

	Binder binder(L);
	TextFieldBase* textField = static_cast<TextFieldBase*>(binder.getInstance("TextField", 1));

    const float *cvec=lua_tovector(L,2);
    if (cvec) {
        textField->setTextColor(cvec[0],cvec[1],cvec[2],cvec[3]);
    }
    else {
        unsigned int color = luaL_optinteger(L, 2, 0xffffff);
        lua_Number alpha = luaL_optnumber(L, 3, 1.0);
        int r = (color >> 16) & 0xff;
        int g = (color >> 8) & 0xff;
        int b = color & 0xff;
        textField->setTextColor(r/255.f,g/255.f,b/255.f,alpha);
    }

	return 0;
}


int TextFieldBinder::getLetterSpacing(lua_State* L)
{
	StackChecker checker(L, "TextFieldBinder::getLetterSpacing", 1);

	Binder binder(L);
	TextFieldBase* textField = static_cast<TextFieldBase*>(binder.getInstance("TextField", 1));

    lua_pushnumber(L, textField->letterSpacing());

	return 1;
}

int TextFieldBinder::setLetterSpacing(lua_State* L)
{
	StackChecker checker(L, "TextFieldBinder::setLetterSpacing", 0);

	Binder binder(L);
	TextFieldBase* textField = static_cast<TextFieldBase*>(binder.getInstance("TextField", 1));

    textField->setLetterSpacing(luaL_checknumber(L, 2));

	return 0;
}

int TextFieldBinder::getLineHeight(lua_State* L)
{
    StackChecker checker(L, "TextFieldBinder::getLineHeight", 1);

    Binder binder(L);
    TextFieldBase* textField = static_cast<TextFieldBase*>(binder.getInstance("TextField", 1));

    lua_pushnumber(L, textField->lineHeight());

    return 1;
}

int TextFieldBinder::getSample(lua_State* L)
{
    StackChecker checker(L, "TextFieldBinder::getSample", 1);

    Binder binder(L);
    TextFieldBase* textField = static_cast<TextFieldBase*>(binder.getInstance("TextField", 1));

    lua_pushstring(L, textField->sample());

    return 1;
}

int TextFieldBinder::setSample(lua_State* L)
{
    StackChecker checker(L, "TextFieldBinder::setSample", 0);

    Binder binder(L);
    TextFieldBase* textField = static_cast<TextFieldBase*>(binder.getInstance("TextField", 1));

    const char* sample = luaL_checkstring(L, 2);
    textField->setSample(sample);

    return 0;
}

int TextFieldBinder::setLayout(lua_State *L)
{
    Binder binder(L);
    TextFieldBase* textField = static_cast<TextFieldBase*>(binder.getInstance("TextField", 1));

    FontBase::TextLayoutParameters tp;
    populateLayout(L,2,&tp);
    textField->setLayout(&tp);

    return 0;
}

int TextFieldBinder::getLayout(lua_State *L)
{
    Binder binder(L);
    TextFieldBase* textField = static_cast<TextFieldBase*>(binder.getInstance("TextField", 1));
    FontBase::TextLayoutParameters tp=textField->getLayout();
    lua_createtable(L,0,6);
    lua_pushnumber(L,tp.w); lua_setfield(L,-2,"w");
    lua_pushnumber(L,tp.h); lua_setfield(L,-2,"h");
    lua_pushinteger(L,tp.flags); lua_setfield(L,-2,"flags");
    lua_pushnumber(L,tp.lineSpacing); lua_setfield(L,-2,"lineSpacing");
    lua_pushnumber(L,tp.letterSpacing); lua_setfield(L,-2,"letterSpacing");
    lua_pushnumber(L,tp.tabSpace); lua_setfield(L,-2,"tabSpace");
    lua_pushstring(L,tp.breakchar.c_str()); lua_setfield(L,-2,"breakChar");

    return 1;
}

int TextFieldBinder::getPointFromTextPosition(lua_State *L)
{
    Binder binder(L);
    TextFieldBase* textField = static_cast<TextFieldBase*>(binder.getInstance("TextField", 1));
    float cx=0,cy=0;
	textField->getPointFromTextPos((size_t)luaL_checkinteger(L,2),cx,cy);
	lua_pushnumber(L,cx);
	lua_pushnumber(L,cy);
	return 2;
}

int TextFieldBinder::getTextPositionFromPoint(lua_State *L)
{
    Binder binder(L);
    TextFieldBase* textField = static_cast<TextFieldBase*>(binder.getInstance("TextField", 1));
	float cx=luaL_checknumber(L,2);
	float cy=luaL_checknumber(L,3);
	size_t p=textField->getTextPosFromPoint(cx,cy);
	lua_pushinteger(L,p);
	lua_pushnumber(L,cx);
	lua_pushnumber(L,cy);
	return 3;
}


