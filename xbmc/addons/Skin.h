#pragma once



#include "Addon.h"
#include "guilib/GraphicContext.h" // needed for the RESOLUTION members
#include "guilib/GUIIncludes.h"    // needed for the GUIInclude member
#define CREDIT_LINE_LENGTH 50

class TiXmlNode;

namespace ADDON
{

class CSkinInfo : public CAddon
{
public:
  class CStartupWindow
  {
  public:
    CStartupWindow(int id, const CStdString &name)
    {
      m_id = id; m_name = name;
    };
    int m_id;
    CStdString m_name;
  };

  //FIXME remove this, kept for current repo handling
  CSkinInfo(const ADDON::AddonProps &props, const RESOLUTION_INFO &res = RESOLUTION_INFO());
  CSkinInfo(const cp_extension_t *ext);
  virtual ~CSkinInfo();

  /*! \brief Load information regarding the skin from the given skin directory
   \param skinDir folder of the skin to load (defaults to this skin's basedir)
   */
  void Start(const CStdString& skinDir = "");

  bool HasSkinFile(const CStdString &strFile) const;

  /*! \brief Get the full path to the specified file in the skin
   We search for XML files in the skin folder that best matches the current resolution.
   \param file XML file to look for
   \param res [out] If non-NULL, the resolution that the returned XML file is in is returned.  Defaults to NULL.
   \param baseDir [in] If non-empty, the given directory is searched instead of the skin's directory.  Defaults to empty.
   \return path to the XML file
   */
  CStdString GetSkinPath(const CStdString& file, RESOLUTION_INFO *res = NULL, const CStdString& baseDir = "") const;

  double GetVersion() const { return m_Version; };

  /*! \brief Return whether skin debugging is enabled
   \return true if skin debugging (set via <debugging>true</debugging> in skin.xml) is enabled.
   */
  bool IsDebugging() const { return m_debugging; };

  /*! \brief Get the id of the first window to load
   The first window is generally Startup.xml unless it doesn't exist or if the skinner
   has specified which start windows they support and the user is going to somewhere other
   than the home screen.
   \return id of the first window to load
   */
  int GetFirstWindow() const;

  /*! \brief Get the id of the window the user wants to start in after any skin animation
   \return id of the start window
   */
  int GetStartWindow() const;

  /*! \brief Translate a resolution string
   \param name the string to translate
   \param res [out] the resolution structure if name is valid
   \return true if the resolution is valid, false otherwise
   */
  static bool TranslateResolution(const CStdString &name, RESOLUTION_INFO &res);

  void ResolveIncludes(TiXmlElement *node);

  float GetEffectsSlowdown() const { return m_effectsSlowDown; };

  const std::vector<CStartupWindow> &GetStartupWindows() const { return m_startupWindows; };

  /*! \brief Retrieve the skin paths to search for skin XML files
   \param paths [out] vector of paths to search, in order.
   */
  void GetSkinPaths(std::vector<CStdString> &paths) const;

  bool IsInUse() const;

//  static bool Check(const CStdString& strSkinDir); // checks if everything is present and accounted for without loading the skin
  static double GetMinVersion();
  void LoadIncludes();
  const INFO::CSkinVariableString* CreateSkinVariable(const CStdString& name, int context);
protected:
  /*! \brief Given a resolution, retrieve the corresponding directory name
   \param res RESOLUTION to translate
   \return directory name for res
   */
  CStdString GetDirFromRes(RESOLUTION res) const;

  /*! \brief grab a resolution tag from a skin's configuration data
   \param props passed addoninfo structure to check for resolution
   \param tag name of the tag to look for
   \param res resolution to return
   \return true if we find a valid resolution, false otherwise
   */
  void GetDefaultResolution(const cp_extension_t *ext, const char *tag, RESOLUTION &res, const RESOLUTION &def) const;

  bool LoadStartupWindows(const cp_extension_t *ext);

  RESOLUTION_INFO m_defaultRes;
  std::vector<RESOLUTION_INFO> m_resolutions;

  double m_Version;

  float m_effectsSlowDown;
  CGUIIncludes m_includes;

  std::vector<CStartupWindow> m_startupWindows;
  bool m_onlyAnimateToHome;
  bool m_debugging;
};

} /*namespace ADDON*/

extern boost::shared_ptr<ADDON::CSkinInfo> g_SkinInfo;
