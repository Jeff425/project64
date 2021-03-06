/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"

CPlugins::CPlugins (const stdstr & PluginDir):
	m_PluginDir(PluginDir),
	m_Gfx(NULL), m_Audio(NULL), m_RSP(NULL), m_Control(NULL),
	m_RenderWindow(NULL), m_DummyWindow(NULL)
{
	CreatePlugins();
	g_Settings->RegisterChangeCB(Plugin_RSP_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	g_Settings->RegisterChangeCB(Plugin_GFX_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	g_Settings->RegisterChangeCB(Plugin_AUDIO_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	g_Settings->RegisterChangeCB(Plugin_CONT_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	g_Settings->RegisterChangeCB(Plugin_UseHleGfx,this,(CSettings::SettingChangedFunc)PluginChanged);
	g_Settings->RegisterChangeCB(Plugin_UseHleAudio,this,(CSettings::SettingChangedFunc)PluginChanged);
	g_Settings->RegisterChangeCB(Game_EditPlugin_Gfx,this,(CSettings::SettingChangedFunc)PluginChanged);
	g_Settings->RegisterChangeCB(Game_EditPlugin_Audio,this,(CSettings::SettingChangedFunc)PluginChanged);
	g_Settings->RegisterChangeCB(Game_EditPlugin_Contr,this,(CSettings::SettingChangedFunc)PluginChanged);
	g_Settings->RegisterChangeCB(Game_EditPlugin_RSP,this,(CSettings::SettingChangedFunc)PluginChanged);
}

CPlugins::~CPlugins (void) 
{
	g_Settings->UnregisterChangeCB(Plugin_RSP_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	g_Settings->UnregisterChangeCB(Plugin_GFX_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	g_Settings->UnregisterChangeCB(Plugin_AUDIO_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	g_Settings->UnregisterChangeCB(Plugin_CONT_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	g_Settings->UnregisterChangeCB(Plugin_UseHleGfx,this,(CSettings::SettingChangedFunc)PluginChanged);
	g_Settings->UnregisterChangeCB(Plugin_UseHleAudio,this,(CSettings::SettingChangedFunc)PluginChanged);
	g_Settings->UnregisterChangeCB(Game_EditPlugin_Gfx,this,(CSettings::SettingChangedFunc)PluginChanged);
	g_Settings->UnregisterChangeCB(Game_EditPlugin_Audio,this,(CSettings::SettingChangedFunc)PluginChanged);
	g_Settings->UnregisterChangeCB(Game_EditPlugin_Contr,this,(CSettings::SettingChangedFunc)PluginChanged);
	g_Settings->UnregisterChangeCB(Game_EditPlugin_RSP,this,(CSettings::SettingChangedFunc)PluginChanged);

	DestroyGfxPlugin();
	DestroyAudioPlugin();
	DestroyRspPlugin();
	DestroyControlPlugin();
}

void CPlugins::PluginChanged ( CPlugins * _this )
{
	if (g_Settings->LoadBool(Game_TempLoaded) == true)
	{
		return;
	}
	bool bGfxChange = _stricmp(_this->m_GfxFile.c_str(),g_Settings->LoadString(Game_Plugin_Gfx).c_str()) != 0;
	bool bAudioChange = _stricmp(_this->m_AudioFile.c_str(),g_Settings->LoadString(Game_Plugin_Audio).c_str()) != 0;
	bool bRspChange = _stricmp(_this->m_RSPFile.c_str(),g_Settings->LoadString(Game_Plugin_RSP).c_str()) != 0;
	bool bContChange = _stricmp(_this->m_ControlFile.c_str(),g_Settings->LoadString(Game_Plugin_Controller).c_str()) != 0;
	
	if ( bGfxChange || bAudioChange || bRspChange || bContChange )
	{
		if (g_Settings->LoadBool(GameRunning_CPU_Running))
		{
			//Ensure that base system actually exists before we go triggering the event
			if (g_BaseSystem)
			{
				g_BaseSystem->ExternalEvent(SysEvent_ChangePlugins);
			}
		}
		else
		{
			_this->Reset(NULL);
			g_Notify->RefreshMenu();
		}
	}
}

template <typename plugin_type>
static void LoadPlugin (SettingID PluginSettingID, SettingID PluginVerSettingID, plugin_type * & plugin, const char * PluginDir, stdstr & FileName, TraceType TraceLevel, const char * type) 
{ 
	if (plugin != NULL)
	{
		return;
	}
	FileName = g_Settings->LoadString(PluginSettingID);
	CPath PluginFileName(PluginDir,FileName.c_str());
	plugin = new plugin_type();
	if (plugin)
	{
		WriteTraceF(TraceLevel,__FUNCTION__ ": %s Loading (%s): Starting",type,(LPCTSTR)PluginFileName);
		if (plugin->Load(PluginFileName))
		{
			WriteTraceF(TraceLevel,__FUNCTION__ ": %s Current Ver: %s",type,plugin->PluginName());
			g_Settings->SaveString(PluginVerSettingID,plugin->PluginName());
		}
		else
		{
			WriteTraceF(TraceError,__FUNCTION__ ": Failed to load %s",(LPCTSTR)PluginFileName);
			delete plugin;
			plugin = NULL;
		}
		WriteTraceF(TraceLevel,__FUNCTION__ ": %s Loading Done",type);
	} 
	else
	{
		WriteTraceF(TraceError,__FUNCTION__ ": Failed to allocate %s plugin",type);
	}
}

void CPlugins::CreatePlugins( void ) 
{
	LoadPlugin(Game_Plugin_Gfx, Plugin_GFX_CurVer, m_Gfx, m_PluginDir.c_str(), m_GfxFile, TraceGfxPlugin, "GFX");
	LoadPlugin(Game_Plugin_Audio, Plugin_AUDIO_CurVer, m_Audio, m_PluginDir.c_str(), m_AudioFile, TraceDebug, "Audio");
	LoadPlugin(Game_Plugin_RSP, Plugin_RSP_CurVer, m_RSP, m_PluginDir.c_str(), m_RSPFile, TraceRSP, "RSP");
	LoadPlugin(Game_Plugin_Controller, Plugin_CONT_CurVer, m_Control, m_PluginDir.c_str(), m_ControlFile, TraceDebug, "Control");

	//Enable debugger
	if (m_RSP != NULL && m_RSP->EnableDebugging)
	{
		WriteTrace(TraceRSP,__FUNCTION__ ": EnableDebugging starting");
		m_RSP->EnableDebugging(bHaveDebugger());
		WriteTrace(TraceRSP,__FUNCTION__ ": EnableDebugging done");
	}

	if (bHaveDebugger())
	{
		g_Notify->RefreshMenu();
	}
}

void CPlugins::GameReset  ( void )
{
	if (m_Gfx)
	{
		m_Gfx->GameReset();
	}
	if (m_Audio)
	{
		m_Audio->GameReset();
	}
	if (m_RSP)
	{
		m_RSP->GameReset();
	}
	if (m_Control)
	{
		m_Control->GameReset();
	}
}

void CPlugins::DestroyGfxPlugin( void ) 
{
	if (m_Gfx == NULL)
	{
		return;
	}
	WriteTrace(TraceGfxPlugin,__FUNCTION__ ": before delete m_Gfx");
	delete m_Gfx;   
	WriteTrace(TraceGfxPlugin,__FUNCTION__ ": after delete m_Gfx");
	m_Gfx = NULL;
//		g_Settings->UnknownSetting_GFX = NULL;
	DestroyRspPlugin();
}

void CPlugins::DestroyAudioPlugin( void ) 
{
	if (m_Audio == NULL)
	{
		return;
	}
	WriteTrace(TraceDebug,__FUNCTION__ ": 5");
	m_Audio->Close();
	WriteTrace(TraceDebug,__FUNCTION__ ": 6");
	delete m_Audio;   
	WriteTrace(TraceDebug,__FUNCTION__ ": 7");
	m_Audio = NULL;
	WriteTrace(TraceDebug,__FUNCTION__ ": 8");
//		g_Settings->UnknownSetting_AUDIO = NULL;
	DestroyRspPlugin();
}

void CPlugins::DestroyRspPlugin( void ) 
{
	if (m_RSP == NULL)
	{
		return;
	}
	WriteTrace(TraceDebug,__FUNCTION__ ": 9");
	m_RSP->Close();
	WriteTrace(TraceDebug,__FUNCTION__ ": 10");
	delete m_RSP;   
	WriteTrace(TraceDebug,__FUNCTION__ ": 11");
	m_RSP = NULL;
	WriteTrace(TraceDebug,__FUNCTION__ ": 12");
//		g_Settings->UnknownSetting_RSP = NULL;
}

void CPlugins::DestroyControlPlugin( void ) 
{
	if (m_Control == NULL) 
	{
		return;
	}
	WriteTrace(TraceDebug,__FUNCTION__ ": 13");
	m_Control->Close();
	WriteTrace(TraceDebug,__FUNCTION__ ": 14");
	delete m_Control;
	WriteTrace(TraceDebug,__FUNCTION__ ": 15");
	m_Control = NULL;
	WriteTrace(TraceDebug,__FUNCTION__ ": 16");
//		g_Settings->UnknownSetting_CTRL = NULL;
}

void CPlugins::SetRenderWindows( CMainGui * RenderWindow, CMainGui * DummyWindow )
{
	m_RenderWindow = RenderWindow;
	m_DummyWindow  = DummyWindow;
}

void CPlugins::RomOpened ( void ) 
{
	m_Gfx->RomOpened();
	m_RSP->RomOpened();
	m_Audio->RomOpened();
	m_Control->RomOpened();
}

void CPlugins::RomClosed ( void ) 
{
	m_Gfx->RomClose();
	m_RSP->RomClose();
	m_Audio->RomClose();
	m_Control->RomClose();
}

bool CPlugins::Initiate ( CN64System * System ) 
{
	WriteTrace(TraceDebug,__FUNCTION__ ": Start");
	//Check to make sure we have the plugin available to be used
	if (m_Gfx   == NULL) { return false; }
	if (m_Audio == NULL) { return false; }
	if (m_RSP   == NULL) { return false; }
	if (m_Control == NULL) { return false; }

	WriteTrace(TraceGfxPlugin,__FUNCTION__ ": Gfx Initiate Starting");
	if (!m_Gfx->Initiate(System,m_RenderWindow))   { return false; }
	WriteTrace(TraceGfxPlugin,__FUNCTION__ ": Gfx Initiate Done");
	WriteTrace(TraceDebug,__FUNCTION__ ": Audio Initiate Starting");
	if (!m_Audio->Initiate(System,m_RenderWindow)) { return false; }
	WriteTrace(TraceDebug,__FUNCTION__ ": Audio Initiate Done");
	WriteTrace(TraceDebug, __FUNCTION__ ": Control Initiate Starting");
	if (!m_Control->Initiate(System,m_RenderWindow)) { return false; }
	WriteTrace(TraceDebug, __FUNCTION__ ": Control Initiate Done");
	WriteTrace(TraceRSP,__FUNCTION__ ": RSP Initiate Starting");
	if (!m_RSP->Initiate(this,System))   { return false; }
	WriteTrace(TraceRSP,__FUNCTION__ ": RSP Initiate Done");
	WriteTrace(TraceDebug,__FUNCTION__ ": Done");	
	return true;
}

bool CPlugins::ResetInUiThread ( CN64System * System )
{
	return m_RenderWindow->ResetPlugins(this, System);
}

bool CPlugins::Reset ( CN64System * System ) 
{
	WriteTrace(TraceDebug,__FUNCTION__ ": Start");	

	bool bGfxChange = _stricmp(m_GfxFile.c_str(),g_Settings->LoadString(Game_Plugin_Gfx).c_str()) != 0;
	bool bAudioChange = _stricmp(m_AudioFile.c_str(),g_Settings->LoadString(Game_Plugin_Audio).c_str()) != 0;
	bool bRspChange = _stricmp(m_RSPFile.c_str(),g_Settings->LoadString(Game_Plugin_RSP).c_str()) != 0;
	bool bContChange = _stricmp(m_ControlFile.c_str(),g_Settings->LoadString(Game_Plugin_Controller).c_str()) != 0;

	//if GFX and Audio has changed we also need to force reset of RSP
	if (bGfxChange || bAudioChange)
		bRspChange = true;

	if (bGfxChange) { DestroyGfxPlugin(); }
	if (bAudioChange) { DestroyAudioPlugin(); }
	if (bRspChange) { DestroyRspPlugin(); }
	if (bContChange) { DestroyControlPlugin(); }

	CreatePlugins();

	if (bGfxChange) 
	{
		WriteTrace(TraceGfxPlugin,__FUNCTION__ ": Gfx Initiate Starting");
		if (!m_Gfx->Initiate(System,m_RenderWindow))   { return false; }
		WriteTrace(TraceGfxPlugin,__FUNCTION__ ": Gfx Initiate Done");
	}
	if (bAudioChange) 
	{
		WriteTrace(TraceDebug,__FUNCTION__ ": Audio Initiate Starting");
		if (!m_Audio->Initiate(System,m_RenderWindow)) { return false; }
		WriteTrace(TraceDebug,__FUNCTION__ ": Audio Initiate Done");
	}
	if (bContChange)
	{
		WriteTrace(TraceDebug, __FUNCTION__ ": Control Initiate Starting");
		if (!m_Control->Initiate(System,m_RenderWindow)) { return false; }
		WriteTrace(TraceDebug, __FUNCTION__ ": Control Initiate Done");
	}
	if (bRspChange) 
	{
		WriteTrace(TraceRSP,__FUNCTION__ ": RSP Initiate Starting");
		if (!m_RSP->Initiate(this,System))   { return false; }
		WriteTrace(TraceRSP,__FUNCTION__ ": RSP Initiate Done");
	}
	WriteTrace(TraceDebug,__FUNCTION__ ": Done");	
	return true;
}

void CPlugins::ConfigPlugin ( DWORD hParent, PLUGIN_TYPE Type ) {
	switch (Type) {
	case PLUGIN_TYPE_RSP:
		if (m_RSP == NULL || m_RSP->DllConfig == NULL) { break; }
		if (!m_RSP->Initilized()) {
			if (!m_RSP->Initiate(NULL,NULL)) {				
				break;
			}
		}
		m_RSP->DllConfig(hParent);
		break;
	case PLUGIN_TYPE_GFX:
		if (m_Gfx == NULL || m_Gfx->DllConfig == NULL) { break; }
		if (!m_Gfx->Initilized()) {
			if (!m_Gfx->Initiate(NULL,m_DummyWindow)) {
				break;
			}
		}
		m_Gfx->DllConfig(hParent);
		break;
	case PLUGIN_TYPE_AUDIO:
		if (m_Audio == NULL || m_Audio->DllConfig == NULL) { break; }
		if (!m_Audio->Initilized()) {
			if (!m_Audio->Initiate(NULL,m_DummyWindow)) {
				break;
			}
		}
		m_Audio->DllConfig(hParent);
		break;
	case PLUGIN_TYPE_CONTROLLER:
		if (m_Control == NULL || m_Control->DllConfig == NULL) { break; }
		if (!m_Control->Initilized()) {
			if (!m_Control->Initiate(NULL,m_DummyWindow)) {
				break;
			}
		}
		m_Control->DllConfig(hParent);
		break;
	}
}

void DummyCheckInterrupts ( void ) {
}

void DummyFunction (void) {
}

#include <windows.h>

void CPlugins::CreatePluginDir ( const stdstr & DstDir ) const {
   char path_buffer[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR], 
	   fname[_MAX_FNAME], ext[_MAX_EXT];
	_splitpath(DstDir.c_str(), drive, dir, fname, ext );			
	_makepath(path_buffer, drive, dir, "", "" );
	if (CreateDirectory(path_buffer,NULL) == 0 && GetLastError() == ERROR_PATH_NOT_FOUND) 
	{
		path_buffer[strlen(path_buffer) - 1] = 0;
		CreatePluginDir(stdstr(path_buffer));
		CreateDirectory(path_buffer,NULL);
	}
}

bool CPlugins::CopyPlugins (  const stdstr & DstDir ) const 
{	
	//Copy GFX Plugin
	CPath srcGfxPlugin(m_PluginDir.c_str(),g_Settings->LoadString(Game_Plugin_Gfx).c_str());
	CPath dstGfxPlugin(DstDir.c_str(),g_Settings->LoadString(Game_Plugin_Gfx).c_str());
	
	if (CopyFile(srcGfxPlugin,dstGfxPlugin,false) == 0) 
	{
		if (GetLastError() == ERROR_PATH_NOT_FOUND) { dstGfxPlugin.CreateDirectory(); }
		if (!CopyFile(srcGfxPlugin,dstGfxPlugin,false)) 
		{
			return false;
		}
	}

	//Copy m_Audio Plugin
	CPath srcAudioPlugin(m_PluginDir.c_str(),g_Settings->LoadString(Game_Plugin_Audio).c_str());
	CPath dstAudioPlugin(DstDir.c_str(), g_Settings->LoadString(Game_Plugin_Audio).c_str());
	if (CopyFile(srcAudioPlugin,dstAudioPlugin,false) == 0) {
		if (GetLastError() == ERROR_PATH_NOT_FOUND) { dstAudioPlugin.CreateDirectory(); }
		if (!CopyFile(srcAudioPlugin,dstAudioPlugin,false))
		{
			return false;
		}
	}

	//Copy RSP Plugin
	CPath srcRSPPlugin(m_PluginDir.c_str(), g_Settings->LoadString(Game_Plugin_RSP).c_str());
	CPath dstRSPPlugin(DstDir.c_str(),g_Settings->LoadString(Game_Plugin_RSP).c_str());
	if (CopyFile(srcRSPPlugin,dstRSPPlugin,false) == 0) {
		if (GetLastError() == ERROR_PATH_NOT_FOUND) { dstRSPPlugin.CreateDirectory(); }
		if (!CopyFile(srcRSPPlugin,dstRSPPlugin,false))
		{
			return false;
		}
	}

	//Copy Controller Plugin
	CPath srcContPlugin(m_PluginDir.c_str(), g_Settings->LoadString(Game_Plugin_Controller).c_str());
	CPath dstContPlugin(DstDir.c_str(),g_Settings->LoadString(Game_Plugin_Controller).c_str());
	if (!srcContPlugin.CopyTo(dstContPlugin))
	{
		if (GetLastError() == ERROR_PATH_NOT_FOUND) { dstContPlugin.CreateDirectory(); }
		if (!CopyFile(srcContPlugin,dstContPlugin,false))
		{
			DWORD dwError = GetLastError();
			dwError = dwError;
			return false;
		}
	}
	return true;
}
