

#include "GUISettingsSliderControl.h"

CGUISettingsSliderControl::CGUISettingsSliderControl(int parentID, int controlID, float posX, float posY, float width, float height, float sliderWidth, float sliderHeight, const CTextureInfo &textureFocus, const CTextureInfo &textureNoFocus, const CTextureInfo& backGroundTexture, const CTextureInfo& nibTexture, const CTextureInfo& nibTextureFocus, const CLabelInfo &labelInfo, int iType)
    : CGUISliderControl(parentID, controlID, posX, posY, sliderWidth, sliderHeight, backGroundTexture, nibTexture,nibTextureFocus, iType)
    , m_buttonControl(parentID, controlID, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo)
    , m_label(posX, posY, width, height, labelInfo)
{
  m_label.SetAlign(XBFONT_CENTER_Y | XBFONT_RIGHT);  
  ControlType = GUICONTROL_SETTINGS_SLIDER;
}

CGUISettingsSliderControl::~CGUISettingsSliderControl(void)
{
}

void CGUISettingsSliderControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_bInvalidated)
  {
    float sliderPosX = m_buttonControl.GetXPosition() + m_buttonControl.GetWidth() - m_width - m_buttonControl.GetLabelInfo().offsetX;
    float sliderPosY = m_buttonControl.GetYPosition() + (m_buttonControl.GetHeight() - m_height) * 0.5f;
    CGUISliderControl::SetPosition(sliderPosX, sliderPosY);
  }
  m_buttonControl.SetFocus(HasFocus());
  m_buttonControl.SetPulseOnSelect(m_pulseOnSelect);
  m_buttonControl.SetEnabled(m_enabled);
  m_buttonControl.DoProcess(currentTime, dirtyregions);
  ProcessText();
  CGUISliderControl::Process(currentTime, dirtyregions);
}

void CGUISettingsSliderControl::Render()
{
  m_buttonControl.Render();
  CGUISliderControl::Render();
  m_label.Render();
}

void CGUISettingsSliderControl::ProcessText()
{
  bool changed = false;

  changed |= m_label.SetMaxRect(m_buttonControl.GetXPosition(), m_posY, m_posX - m_buttonControl.GetXPosition(), m_height);
  changed |= m_label.SetText(CGUISliderControl::GetDescription());
  if (IsDisabled())
    changed |= m_label.SetColor(CGUILabel::COLOR_DISABLED);
  else if (HasFocus())
    changed |= m_label.SetColor(CGUILabel::COLOR_FOCUSED);
  else
    changed |= m_label.SetColor(CGUILabel::COLOR_TEXT);

  if (changed)
    MarkDirtyRegion();
}

bool CGUISettingsSliderControl::OnAction(const CAction &action)
{
  return CGUISliderControl::OnAction(action);
}

void CGUISettingsSliderControl::FreeResources(bool immediately)
{
  CGUISliderControl::FreeResources(immediately);
  m_buttonControl.FreeResources(immediately);
}

void CGUISettingsSliderControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUISliderControl::DynamicResourceAlloc(bOnOff);
  m_buttonControl.DynamicResourceAlloc(bOnOff);
}

void CGUISettingsSliderControl::AllocResources()
{
  CGUISliderControl::AllocResources();
  m_buttonControl.AllocResources();
}

void CGUISettingsSliderControl::SetInvalid()
{
  CGUISliderControl::SetInvalid();
  m_buttonControl.SetInvalid();
}

void CGUISettingsSliderControl::SetPosition(float posX, float posY)
{
  m_buttonControl.SetPosition(posX, posY);
  CGUISliderControl::SetInvalid();
}

void CGUISettingsSliderControl::SetWidth(float width)
{
  m_buttonControl.SetWidth(width);
  CGUISliderControl::SetInvalid();
}

void CGUISettingsSliderControl::SetHeight(float height)
{
  m_buttonControl.SetHeight(height);
  CGUISliderControl::SetInvalid();
}

void CGUISettingsSliderControl::SetEnabled(bool bEnable)
{
  CGUISliderControl::SetEnabled(bEnable);
  m_buttonControl.SetEnabled(bEnable);
}

CStdString CGUISettingsSliderControl::GetDescription() const
{
  return m_buttonControl.GetDescription() + " " + CGUISliderControl::GetDescription();
}

bool CGUISettingsSliderControl::UpdateColors()
{
  bool changed = CGUISliderControl::UpdateColors();
  changed |= m_buttonControl.UpdateColors();

  return changed;
}
