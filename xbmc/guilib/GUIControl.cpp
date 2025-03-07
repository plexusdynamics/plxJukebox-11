

#include "GUIControl.h"

#include "GUIInfoManager.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIWindowManager.h"
#include "GUIControlProfiler.h"
#include "input/MouseStat.h"
#include "Key.h"

using namespace std;

CGUIControl::CGUIControl()
{
  m_hasRendered = false;
  m_bHasFocus = false;
  m_controlID = 0;
  m_parentID = 0;
  m_visible = VISIBLE;
  m_visibleFromSkinCondition = true;
  m_forceHidden = false;
  m_visibleCondition = 0;
  m_enableCondition = 0;
  m_enabled = true;
  m_diffuseColor = 0xffffffff;
  m_posX = 0;
  m_posY = 0;
  m_width = 0;
  m_height = 0;
  ControlType = GUICONTROL_UNKNOWN;
  m_bInvalidated = true;
  m_bAllocated=false;
  m_parentControl = NULL;
  m_hasCamera = false;
  m_pushedUpdates = false;
  m_pulseOnSelect = false;
  m_controlIsDirty = true;
}

CGUIControl::CGUIControl(int parentID, int controlID, float posX, float posY, float width, float height)
: m_hitRect(posX, posY, posX + width, posY + height)
{
  m_posX = posX;
  m_posY = posY;
  m_width = width;
  m_height = height;
  m_bHasFocus = false;
  m_controlID = controlID;
  m_parentID = parentID;
  m_visible = VISIBLE;
  m_visibleFromSkinCondition = true;
  m_diffuseColor = 0xffffffff;
  m_forceHidden = false;
  m_visibleCondition = 0;
  m_enableCondition = 0;
  m_enabled = true;
  ControlType = GUICONTROL_UNKNOWN;
  m_bInvalidated = true;
  m_bAllocated=false;
  m_hasRendered = false;
  m_parentControl = NULL;
  m_hasCamera = false;
  m_pushedUpdates = false;
  m_pulseOnSelect = false;
}


CGUIControl::~CGUIControl(void)
{

}

void CGUIControl::AllocResources()
{
  m_hasRendered = false;
  m_bInvalidated = true;
  m_bAllocated=true;
}

void CGUIControl::FreeResources(bool immediately)
{
  if (m_bAllocated)
  {
    // Reset our animation states - not conditional anims though.
    // I'm not sure if this is needed for most cases anyway.  I believe it's only here
    // because some windows aren't loaded on demand
    for (unsigned int i = 0; i < m_animations.size(); i++)
    {
      CAnimation &anim = m_animations[i];
      if (anim.GetType() != ANIM_TYPE_CONDITIONAL)
        anim.ResetAnimation();
    }
    m_bAllocated=false;
  }
  m_hasRendered = false;
}

void CGUIControl::DynamicResourceAlloc(bool bOnOff)
{

}

// the main processing routine.
// 1. animate and set animation transform
// 2. if visible, process
// 3. reset the animation transform
void CGUIControl::DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  CRect dirtyRegion = m_renderRegion;

  bool changed = m_bInvalidated;

  changed |= Animate(currentTime);

  m_cachedTransform = g_graphicsContext.AddTransform(m_transform);
  if (m_hasCamera)
    g_graphicsContext.SetCameraPosition(m_camera);

  if (IsVisible())
    Process(currentTime, dirtyregions);

  changed |=  m_controlIsDirty;

  if (changed || dirtyRegion != m_renderRegion)
  {
    dirtyRegion.Union(m_renderRegion);
    dirtyregions.push_back(dirtyRegion);
  }

  if (m_hasCamera)
    g_graphicsContext.RestoreCameraPosition();
  g_graphicsContext.RemoveTransform();

  m_bInvalidated = false;
  m_controlIsDirty = false;
}

void CGUIControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  // update our render region
  m_renderRegion = g_graphicsContext.generateAABB(CalcRenderRegion());
}

// the main render routine.
// 1. set the animation transform
// 2. if visible, paint
// 3. reset the animation transform
void CGUIControl::DoRender()
{
  g_graphicsContext.SetTransform(m_cachedTransform);
  if (m_hasCamera)
    g_graphicsContext.SetCameraPosition(m_camera);
  if (IsVisible())
  {
    GUIPROFILER_RENDER_BEGIN(this);
    Render();
    GUIPROFILER_RENDER_END(this);
  }
  if (m_hasCamera)
    g_graphicsContext.RestoreCameraPosition();
  g_graphicsContext.RemoveTransform();
}

void CGUIControl::Render()
{
  m_hasRendered = true;
}

bool CGUIControl::OnAction(const CAction &action)
{
  if (HasFocus())
  {
    switch (action.GetID())
    {
    case ACTION_MOVE_DOWN:
      OnDown();
      return true;

    case ACTION_MOVE_UP:
      OnUp();
      return true;

    case ACTION_MOVE_LEFT:
      OnLeft();
      return true;

    case ACTION_MOVE_RIGHT:
      OnRight();
      return true;

    case ACTION_NAV_BACK:
      return OnBack();

    case ACTION_NEXT_CONTROL:
      OnNextControl();
      return true;

    case ACTION_PREV_CONTROL:
      OnPrevControl();
      return true;
    }
  }
  return false;
}

// Movement controls (derived classes can override)
void CGUIControl::OnUp()
{
  if (HasFocus())
    m_actionUp.Execute(GetID(), GetParentID(), ACTION_MOVE_UP);
}

void CGUIControl::OnDown()
{
  if (HasFocus())
    m_actionDown.Execute(GetID(), GetParentID(), ACTION_MOVE_DOWN);
}

void CGUIControl::OnLeft()
{
  if (HasFocus())
    m_actionLeft.Execute(GetID(), GetParentID(), ACTION_MOVE_LEFT);
}

void CGUIControl::OnRight()
{
  if (HasFocus())
    m_actionRight.Execute(GetID(), GetParentID(), ACTION_MOVE_RIGHT);
}

bool CGUIControl::OnBack()
{
  return HasFocus() ? m_actionBack.Execute(GetID(), GetParentID(), ACTION_NAV_BACK) : false;
}

void CGUIControl::OnNextControl()
{
  if (HasFocus())
    m_actionNext.Execute(GetID(), GetParentID(), ACTION_NEXT_CONTROL);
}

void CGUIControl::OnPrevControl()
{
  if (HasFocus())
    m_actionPrev.Execute(GetID(), GetParentID(), ACTION_PREV_CONTROL);
}

bool CGUIControl::SendWindowMessage(CGUIMessage &message)
{
  CGUIWindow *pWindow = g_windowManager.GetWindow(GetParentID());
  if (pWindow)
    return pWindow->OnMessage(message);
  return g_windowManager.SendMessage(message);
}

int CGUIControl::GetID(void) const
{
  return m_controlID;
}


int CGUIControl::GetParentID(void) const
{
  return m_parentID;
}

bool CGUIControl::HasFocus(void) const
{
  return m_bHasFocus;
}

void CGUIControl::SetFocus(bool focus)
{
  if (m_bHasFocus && !focus)
    QueueAnimation(ANIM_TYPE_UNFOCUS);
  else if (!m_bHasFocus && focus)
    QueueAnimation(ANIM_TYPE_FOCUS);
  m_bHasFocus = focus;
}

bool CGUIControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId() == GetID() )
  {
    switch (message.GetMessage() )
    {
    case GUI_MSG_SETFOCUS:
      // if control is disabled then move 2 the next control
      if ( !CanFocus() )
      {
        CLog::Log(LOGERROR, "Control %u in window %u has been asked to focus, "
                            "but it can't",
                  GetID(), GetParentID());
        return false;
      }
      SetFocus(true);
      {
        // inform our parent window that this has happened
        CGUIMessage message(GUI_MSG_FOCUSED, GetParentID(), GetID());
        if (m_parentControl)
          m_parentControl->OnMessage(message);
      }
      return true;
      break;

    case GUI_MSG_LOSTFOCUS:
      {
        SetFocus(false);
        // and tell our parent so it can unfocus
        if (m_parentControl)
          m_parentControl->OnMessage(message);
        return true;
      }
      break;

    case GUI_MSG_VISIBLE:
      SetVisible(true, true);
      return true;
      break;

    case GUI_MSG_HIDDEN:
      SetVisible(false);
      return true;

      // Note that the skin <enable> tag will override these messages
    case GUI_MSG_ENABLED:
      SetEnabled(true);
      return true;

    case GUI_MSG_DISABLED:
      SetEnabled(false);
      return true;

    case GUI_MSG_WINDOW_RESIZE:
      // invalidate controls to get them to recalculate sizing information
      SetInvalid();
      return true;
    }
  }
  return false;
}

bool CGUIControl::CanFocus() const
{
  if (!IsVisible() && !m_allowHiddenFocus) return false;
  if (IsDisabled()) return false;
  return true;
}

bool CGUIControl::IsVisible() const
{
  if (m_forceHidden) return false;
  return m_visible == VISIBLE;
}

bool CGUIControl::IsDisabled() const
{
  return !m_enabled;
}

void CGUIControl::SetEnabled(bool bEnable)
{
  if (bEnable != m_enabled)
  {
    m_enabled = bEnable;
    SetInvalid();
  }
}

void CGUIControl::SetEnableCondition(const CStdString &expression)
{
  if (expression == "true")
    m_enabled = true;
  else if (expression == "false")
    m_enabled = false;
  else
    m_enableCondition = g_infoManager.Register(expression, GetParentID());
}

void CGUIControl::SetPosition(float posX, float posY)
{
  if ((m_posX != posX) || (m_posY != posY))
  {
    MarkDirtyRegion();

    m_hitRect += CPoint(posX - m_posX, posY - m_posY);
    m_posX = posX;
    m_posY = posY;

    SetInvalid();
  }
}

bool CGUIControl::SetColorDiffuse(const CGUIInfoColor &color)
{
  bool changed = m_diffuseColor != color;
  m_diffuseColor = color;
  return changed;
}

float CGUIControl::GetXPosition() const
{
  return m_posX;
}

float CGUIControl::GetYPosition() const
{
  return m_posY;
}

float CGUIControl::GetWidth() const
{
  return m_width;
}

float CGUIControl::GetHeight() const
{
  return m_height;
}

void CGUIControl::MarkDirtyRegion()
{
  m_controlIsDirty = true;
}

CRect CGUIControl::CalcRenderRegion() const
{
  CPoint tl(GetXPosition(), GetYPosition());
  CPoint br(tl.x + GetWidth(), tl.y + GetHeight());

  return CRect(tl.x, tl.y, br.x, br.y);
}

void CGUIControl::SetNavigation(int up, int down, int left, int right, int back)
{
  m_actionUp.SetNavigation(up);
  m_actionDown.SetNavigation(down);
  m_actionLeft.SetNavigation(left);
  m_actionRight.SetNavigation(right);
  m_actionBack.SetNavigation(back);
}

void CGUIControl::SetTabNavigation(int next, int prev)
{
  m_actionNext.SetNavigation(next);
  m_actionPrev.SetNavigation(prev);
}

void CGUIControl::SetNavigationActions(const CGUIAction &up, const CGUIAction &down,
                                       const CGUIAction &left, const CGUIAction &right,
                                       const CGUIAction &back, bool replace)
{
  if (!m_actionLeft.HasAnyActions()  || replace) m_actionLeft  = left;
  if (!m_actionRight.HasAnyActions() || replace) m_actionRight = right;
  if (!m_actionUp.HasAnyActions()    || replace) m_actionUp    = up;
  if (!m_actionDown.HasAnyActions()  || replace) m_actionDown  = down;
  if (!m_actionBack.HasAnyActions()  || replace) m_actionBack  = back;
}

void CGUIControl::SetWidth(float width)
{
  if (m_width != width)
  {
    MarkDirtyRegion();
    m_width = width;
    m_hitRect.x2 = m_hitRect.x1 + width;
    SetInvalid();
  }
}

void CGUIControl::SetHeight(float height)
{
  if (m_height != height)
  {
    MarkDirtyRegion();
    m_height = height;
    m_hitRect.y2 = m_hitRect.y1 + height;
    SetInvalid();
  }
}

void CGUIControl::SetVisible(bool bVisible, bool setVisState)
{
  if (bVisible && setVisState)
  {  // TODO: currently we only update m_visible from GUI_MSG_VISIBLE (SET_CONTROL_VISIBLE)
     //       otherwise we just set m_forceHidden
    GUIVISIBLE visible = m_visible;
    if (m_visibleCondition)
      visible = g_infoManager.GetBoolValue(m_visibleCondition) ? VISIBLE : HIDDEN;
    else
      visible = VISIBLE;
    if (visible != m_visible)
    {
      m_visible = visible;
      SetInvalid();
    }
  }
  if (m_forceHidden == bVisible)
  {
    m_forceHidden = !bVisible;
    SetInvalid();
  }
  if (m_forceHidden)
  { // reset any visible animations that are in process
    if (IsAnimating(ANIM_TYPE_VISIBLE))
    {
//        CLog::Log(LOGDEBUG, "Resetting visible animation on control %i (we are %s)", m_controlID, m_visible ? "visible" : "hidden");
      CAnimation *visibleAnim = GetAnimation(ANIM_TYPE_VISIBLE);
      if (visibleAnim) visibleAnim->ResetAnimation();
    }
  }
}

bool CGUIControl::HitTest(const CPoint &point) const
{
  return m_hitRect.PtInRect(point);
}

EVENT_RESULT CGUIControl::SendMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  CPoint childPoint(point);
  m_transform.InverseTransformPosition(childPoint.x, childPoint.y);
  if (!CanFocusFromPoint(childPoint))
    return EVENT_RESULT_UNHANDLED;

  bool handled = OnMouseOver(childPoint);
  EVENT_RESULT ret = OnMouseEvent(childPoint, event);
  if (ret)
    return ret;
  return (handled && (event.m_id == ACTION_MOUSE_MOVE)) ? EVENT_RESULT_HANDLED : EVENT_RESULT_UNHANDLED;
}

// override this function to implement custom mouse behaviour
bool CGUIControl::OnMouseOver(const CPoint &point)
{
  if (g_Mouse.GetState() != MOUSE_STATE_DRAG)
    g_Mouse.SetState(MOUSE_STATE_FOCUS);
  if (!CanFocus()) return false;
  CGUIMessage msg(GUI_MSG_SETFOCUS, GetParentID(), GetID());
  OnMessage(msg);
  return true;
}

void CGUIControl::UpdateVisibility(const CGUIListItem *item)
{
  if (m_visibleCondition)
  {
    bool bWasVisible = m_visibleFromSkinCondition;
    m_visibleFromSkinCondition = g_infoManager.GetBoolValue(m_visibleCondition, item);
    if (!bWasVisible && m_visibleFromSkinCondition)
    { // automatic change of visibility - queue the in effect
  //    CLog::Log(LOGDEBUG, "Visibility changed to visible for control id %i", m_controlID);
      QueueAnimation(ANIM_TYPE_VISIBLE);
    }
    else if (bWasVisible && !m_visibleFromSkinCondition)
    { // automatic change of visibility - do the out effect
  //    CLog::Log(LOGDEBUG, "Visibility changed to hidden for control id %i", m_controlID);
      QueueAnimation(ANIM_TYPE_HIDDEN);
    }
  }
  // check for conditional animations
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    if (anim.GetType() == ANIM_TYPE_CONDITIONAL)
      anim.UpdateCondition(item);
  }
  // and check for conditional enabling - note this overrides SetEnabled() from the code currently
  // this may need to be reviewed at a later date
  bool enabled = m_enabled;
  if (m_enableCondition)
    m_enabled = g_infoManager.GetBoolValue(m_enableCondition, item);

  if (m_enabled != enabled)
    MarkDirtyRegion();

  m_allowHiddenFocus.Update(item);
  if (UpdateColors())
    MarkDirtyRegion();
  // and finally, update our control information (if not pushed)
  if (!m_pushedUpdates)
    UpdateInfo(item);
}

bool CGUIControl::UpdateColors()
{
  return m_diffuseColor.Update();
}

void CGUIControl::SetInitialVisibility()
{
  if (m_visibleCondition)
  {
    m_visibleFromSkinCondition = g_infoManager.GetBoolValue(m_visibleCondition);
    m_visible = m_visibleFromSkinCondition ? VISIBLE : HIDDEN;
  //  CLog::Log(LOGDEBUG, "Set initial visibility for control %i: %s", m_controlID, m_visible == VISIBLE ? "visible" : "hidden");
  }
  // and handle animation conditions as well
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    if (anim.GetType() == ANIM_TYPE_CONDITIONAL)
      anim.SetInitialCondition();
  }
  // and check for conditional enabling - note this overrides SetEnabled() from the code currently
  // this may need to be reviewed at a later date
  if (m_enableCondition)
    m_enabled = g_infoManager.GetBoolValue(m_enableCondition);
  m_allowHiddenFocus.Update();
  UpdateColors();

  MarkDirtyRegion();
}

void CGUIControl::SetVisibleCondition(const CStdString &expression, const CStdString &allowHiddenFocus)
{
  if (expression == "true")
    m_visible = VISIBLE;
  else if (expression == "false")
    m_visible = HIDDEN;
  else  // register with the infomanager for updates
    m_visibleCondition = g_infoManager.Register(expression, GetParentID());
  m_allowHiddenFocus.Parse(allowHiddenFocus, GetParentID());
}

void CGUIControl::SetAnimations(const vector<CAnimation> &animations)
{
  m_animations = animations;
  MarkDirtyRegion();
}

void CGUIControl::ResetAnimation(ANIMATION_TYPE type)
{
  MarkDirtyRegion();

  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    if (m_animations[i].GetType() == type)
      m_animations[i].ResetAnimation();
  }
}

void CGUIControl::ResetAnimations()
{
  MarkDirtyRegion();

  for (unsigned int i = 0; i < m_animations.size(); i++)
    m_animations[i].ResetAnimation();

  MarkDirtyRegion();
}

bool CGUIControl::CheckAnimation(ANIMATION_TYPE animType)
{
  // rule out the animations we shouldn't perform
  if (!IsVisible() || !HasRendered())
  { // hidden or never rendered - don't allow exit or entry animations for this control
    if (animType == ANIM_TYPE_WINDOW_CLOSE)
    { // could be animating a (delayed) window open anim, so reset it
      ResetAnimation(ANIM_TYPE_WINDOW_OPEN);
      return false;
    }
  }
  if (!IsVisible())
  { // hidden - only allow hidden anims if we're animating a visible anim
    if (animType == ANIM_TYPE_HIDDEN && !IsAnimating(ANIM_TYPE_VISIBLE))
    {
      // update states to force it hidden
      UpdateStates(animType, ANIM_PROCESS_NORMAL, ANIM_STATE_APPLIED);
      return false;
    }
    if (animType == ANIM_TYPE_WINDOW_OPEN)
      return false;
  }
  return true;
}

void CGUIControl::QueueAnimation(ANIMATION_TYPE animType)
{
  MarkDirtyRegion();
  if (!CheckAnimation(animType))
    return;
  CAnimation *reverseAnim = GetAnimation((ANIMATION_TYPE)-animType, false);
  CAnimation *forwardAnim = GetAnimation(animType);
  // we first check whether the reverse animation is in progress (and reverse it)
  // then we check for the normal animation, and queue it
  if (reverseAnim && reverseAnim->IsReversible() && (reverseAnim->GetState() == ANIM_STATE_IN_PROCESS || reverseAnim->GetState() == ANIM_STATE_DELAYED))
  {
    reverseAnim->QueueAnimation(ANIM_PROCESS_REVERSE);
    if (forwardAnim) forwardAnim->ResetAnimation();
  }
  else if (forwardAnim)
  {
    forwardAnim->QueueAnimation(ANIM_PROCESS_NORMAL);
    if (reverseAnim) reverseAnim->ResetAnimation();
  }
  else
  { // hidden and visible animations delay the change of state.  If there is no animations
    // to perform, then we should just change the state straightaway
    if (reverseAnim) reverseAnim->ResetAnimation();
    UpdateStates(animType, ANIM_PROCESS_NORMAL, ANIM_STATE_APPLIED);
  }
}

CAnimation *CGUIControl::GetAnimation(ANIMATION_TYPE type, bool checkConditions /* = true */)
{
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    if (anim.GetType() == type)
    {
      if (!checkConditions || anim.CheckCondition())
        return &anim;
    }
  }
  return NULL;
}

bool CGUIControl::HasAnimation(ANIMATION_TYPE type)
{
  return (NULL != GetAnimation(type, true));
}

void CGUIControl::UpdateStates(ANIMATION_TYPE type, ANIMATION_PROCESS currentProcess, ANIMATION_STATE currentState)
{
  // Make sure control is hidden or visible at the appropriate times
  // while processing a visible or hidden animation it needs to be visible,
  // but when finished a hidden operation it needs to be hidden
  if (type == ANIM_TYPE_VISIBLE)
  {
    if (currentProcess == ANIM_PROCESS_REVERSE)
    {
      if (currentState == ANIM_STATE_APPLIED)
        m_visible = HIDDEN;
    }
    else if (currentProcess == ANIM_PROCESS_NORMAL)
    {
      if (currentState == ANIM_STATE_DELAYED)
        m_visible = DELAYED;
      else
        m_visible = m_visibleFromSkinCondition ? VISIBLE : HIDDEN;
    }
  }
  else if (type == ANIM_TYPE_HIDDEN)
  {
    if (currentProcess == ANIM_PROCESS_NORMAL)  // a hide animation
    {
      if (currentState == ANIM_STATE_APPLIED)
        m_visible = HIDDEN; // finished
      else
        m_visible = VISIBLE; // have to be visible until we are finished
    }
    else if (currentProcess == ANIM_PROCESS_REVERSE)  // a visible animation
    { // no delay involved here - just make sure it's visible
      m_visible = m_visibleFromSkinCondition ? VISIBLE : HIDDEN;
    }
  }
  else if (type == ANIM_TYPE_WINDOW_OPEN)
  {
    if (currentProcess == ANIM_PROCESS_NORMAL)
    {
      if (currentState == ANIM_STATE_DELAYED)
        m_visible = DELAYED; // delayed
      else
        m_visible = m_visibleFromSkinCondition ? VISIBLE : HIDDEN;
    }
  }
  else if (type == ANIM_TYPE_FOCUS)
  {
    // call the focus function if we have finished a focus animation
    // (buttons can "click" on focus)
    if (currentProcess == ANIM_PROCESS_NORMAL && currentState == ANIM_STATE_APPLIED)
      OnFocus();
  }
  else if (type == ANIM_TYPE_UNFOCUS)
  {
    // call the unfocus function if we have finished a focus animation
    // (buttons can "click" on focus)
    if (currentProcess == ANIM_PROCESS_NORMAL && currentState == ANIM_STATE_APPLIED)
      OnUnFocus();
  }
}

bool CGUIControl::Animate(unsigned int currentTime)
{
  // check visible state outside the loop, as it could change
  GUIVISIBLE visible = m_visible;

  m_transform.Reset();
  bool changed = false;

  CPoint center(GetXPosition() + GetWidth() * 0.5f, GetYPosition() + GetHeight() * 0.5f);
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    anim.Animate(currentTime, HasRendered() || visible == DELAYED);
    // Update the control states (such as visibility)
    UpdateStates(anim.GetType(), anim.GetProcess(), anim.GetState());
    // and render the animation effect
    changed |= (anim.GetProcess() != ANIM_PROCESS_NONE);
    anim.RenderAnimation(m_transform, center);

/*    // debug stuff
    if (anim.currentProcess != ANIM_PROCESS_NONE)
    {
      if (anim.effect == EFFECT_TYPE_ZOOM)
      {
        if (IsVisible())
          CLog::Log(LOGDEBUG, "Animating control %d with a %s zoom effect %s. Amount is %2.1f, visible=%s", m_controlID, anim.type == ANIM_TYPE_CONDITIONAL ? (anim.lastCondition ? "conditional_on" : "conditional_off") : (anim.type == ANIM_TYPE_VISIBLE ? "visible" : "hidden"), anim.currentProcess == ANIM_PROCESS_NORMAL ? "normal" : "reverse", anim.amount, IsVisible() ? "true" : "false");
      }
      else if (anim.effect == EFFECT_TYPE_FADE)
      {
        if (IsVisible())
          CLog::Log(LOGDEBUG, "Animating control %d with a %s fade effect %s. Amount is %2.1f. Visible=%s", m_controlID, anim.type == ANIM_TYPE_CONDITIONAL ? (anim.lastCondition ? "conditional_on" : "conditional_off") : (anim.type == ANIM_TYPE_VISIBLE ? "visible" : "hidden"), anim.currentProcess == ANIM_PROCESS_NORMAL ? "normal" : "reverse", anim.amount, IsVisible() ? "true" : "false");
      }
    }*/
  }

  return changed;
}

bool CGUIControl::IsAnimating(ANIMATION_TYPE animType)
{
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    if (anim.GetType() == animType)
    {
      if (anim.GetQueuedProcess() == ANIM_PROCESS_NORMAL)
        return true;
      if (anim.GetProcess() == ANIM_PROCESS_NORMAL)
        return true;
    }
    else if (anim.GetType() == -animType)
    {
      if (anim.GetQueuedProcess() == ANIM_PROCESS_REVERSE)
        return true;
      if (anim.GetProcess() == ANIM_PROCESS_REVERSE)
        return true;
    }
  }
  return false;
}

int CGUIControl::GetNextControl(int direction) const
{
  switch (direction)
  {
  case ACTION_MOVE_UP:
    return m_actionUp.GetNavigation();
  case ACTION_MOVE_DOWN:
    return m_actionDown.GetNavigation();
  case ACTION_MOVE_LEFT:
    return m_actionLeft.GetNavigation();
  case ACTION_MOVE_RIGHT:
    return m_actionRight.GetNavigation();
  case ACTION_NAV_BACK:
    return m_actionBack.GetNavigation();
  default:
    return -1;
  }
}

bool CGUIControl::CanFocusFromPoint(const CPoint &point) const
{
  return CanFocus() && HitTest(point);
}

void CGUIControl::UnfocusFromPoint(const CPoint &point)
{
  CPoint controlPoint(point);
  m_transform.InverseTransformPosition(controlPoint.x, controlPoint.y);
  if (!HitTest(controlPoint))
    SetFocus(false);
}

bool CGUIControl::HasID(int id) const
{
  return GetID() == id;
}

bool CGUIControl::HasVisibleID(int id) const
{
  return GetID() == id && IsVisible();
}

void CGUIControl::SaveStates(vector<CControlState> &states)
{
  // empty for now - do nothing with the majority of controls
}

void CGUIControl::SetHitRect(const CRect &rect)
{
  m_hitRect = rect;
}

void CGUIControl::SetCamera(const CPoint &camera)
{
  m_camera = camera;
  m_hasCamera = true;
}

CPoint CGUIControl::GetRenderPosition() const
{
  float z = 0;
  CPoint point(GetPosition());
  m_transform.TransformPosition(point.x, point.y, z);
  if (m_parentControl)
    point += m_parentControl->GetRenderPosition();
  return point;
}
