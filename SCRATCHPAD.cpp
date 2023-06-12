
void PluginPaths::initApplePaths() {
  using namespace std;

  const string mac_home = m_home + "/Library/Application Support/OpenCPN";
  m_userLibdir = mac_home + "/Contents/PlugIns";
  m_userBindir = m_userLibdir;
  m_userDatadir = mac_home + "/Contents";

  m_libdirs.push_back(m_userLibdir);
  wxFileName fn_exe(g_BasePlatform->GetExePath());
  fn_exe.RemoveLastDir();
  string exeLibDir =
      fn_exe.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR).ToStdString() +
      "PlugIns";
  m_libdirs.push_back(exeLibDir);
  // m_libdirs.push_back("/Applications/OpenCPN.app/Contents/Plugins");
  m_bindirs = m_libdirs;

  m_datadirs.push_back(m_userDatadir);
  m_datadirs.push_back("/Applications/OpenCPN.app/Contents/PlugIns");
}



PluginPaths::PluginPaths() {
  using namespace std;

  wxString wxHome("unusable-$HOME");
  wxGetEnv("HOME", &wxHome);
  m_home = wxHome.ToStdString();

  auto osSystemId = wxPlatformInfo::Get().GetOperatingSystemId();
  if (osSystemId & wxOS_WINDOWS) {
    initWindowsPaths();
  } else if (g_BasePlatform->isFlatpacked()) {
    initFlatpackPaths();
  } else if (osSystemId & wxOS_UNIX_LINUX) {
#ifdef __OCPN__ANDROID__
    initAndroidPaths();
#else
    initLinuxPaths();
#endif
  } else if (osSystemId & wxOS_MAC) {
    initApplePaths();
  } else {
    wxString os_name = wxPlatformInfo::Get().GetPortIdName();
    wxLogMessage(_T("OS_NAME: ") + os_name);
    if (os_name.Contains(_T("wxQT"))) {
      initAndroidPaths();
    } else
      wxLogWarning("PluginPaths: Unknown platform");
  }
}
