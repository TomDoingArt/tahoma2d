#pragma once

#ifndef PERSPECTIVETOOL_H
#define PERSPECTIVETOOL_H

#include "tproperty.h"
#include "tpersist.h"
#include "tundo.h"
#include "historytypes.h"

#include "toonz/tproject.h"

#include "toonzqt/selection.h"

#include "tools/tool.h"
#include "tools/tooloptions.h"

#include <QCoreApplication>

class SceneViewer;
class PerspectiveTool;

//--------------------------------------------------------------

#define CUSTOM_WSTR L"<custom>"

//--------------------------------------------------------------

enum PerspectiveType { Undefined, VanishingPoint, Line };

//************************************************************************
//  Perspective Data declaration
//************************************************************************

class PerspectiveData {
public:
  PerspectiveData(PerspectiveType perspectiveType, TPointD centerPoint,
                  double rotation, int spacing, bool horizon, double opacity,
                  TPixel32 color, bool parallel)
      : m_type(perspectiveType)
      , m_centerPoint(centerPoint)
      , m_rotation(rotation)
      , m_spacing(spacing)
      , m_horizon(horizon)
      , m_opacity(opacity)
      , m_color(color)
      , m_parallel(parallel){};

  ~PerspectiveData(){};

  virtual void draw(SceneViewer *viewer, TRectD cameraRect){};

  void setType(PerspectiveType pType) { m_type = pType; }
  PerspectiveType getType() { return m_type; }

  void setCenterPoint(TPointD point) { m_centerPoint = point; }
  TPointD getCenterPoint() { return m_centerPoint; }

  void setRotation(double angle) { m_rotation = angle; }
  virtual double getRotation() { return m_rotation; }

  void setParallel(bool isParallel) { m_parallel = isParallel; }
  bool isParallel() { return m_parallel; }

  void setSpacing(double space) { m_spacing = space; }
  virtual double getSpacing() { return m_spacing; }
  void setMinSpacing(int minSpace) { m_minSpacing = minSpace; }
  virtual double getMinSpacing() { return m_minSpacing; }
  void setMaxSpacing(int maxSpace) { m_maxSpacing = maxSpace; }
  virtual double getMaxSpacing() { return m_maxSpacing; }

  void setHorizon(bool isHorizon) { m_horizon = isHorizon; }
  bool isHorizon() { return m_horizon; }

  void setOpacity(double opacity) { m_opacity = opacity; }
  double getOpacity() { return m_opacity; }

  void setColor(TPixel32 color) { m_color = color; }
  TPixel32 getColor() { return m_color; }

protected:
  PerspectiveType m_type;
  TPointD m_centerPoint;
  double m_rotation;
  bool m_parallel;
  double m_spacing, m_minSpacing = 1.0, m_maxSpacing = UINT_MAX;
  bool m_horizon;
  double m_opacity;
  TPixel32 m_color;
};

//************************************************************************
//  Perspective Controls declaration
//************************************************************************

class PerspectiveControls {
  PerspectiveData *m_perspective;
  TPointD m_cursorPos;
  bool m_active;
  bool m_showAdvanced;
  double m_unit = 1;

  double m_handleRadius = 10;
  // Common Controls
  TPointD m_rotationPos = TPointD(40, 0);
  TPointD m_spacingPos  = TPointD(0, -40);

  // Vanishing Point Controls
  TPointD m_leftPivotPos   = TPointD(-40, 0);
  TPointD m_leftHandlePos  = TPointD(-80, 0);
  TPointD m_rightPivotPos  = TPointD(40, 0);
  TPointD m_rightHandlePos = TPointD(80, 0);

public:
  PerspectiveControls(PerspectiveData *perspective)
      : m_perspective(perspective), m_active(false), m_showAdvanced(false) {}
  ~PerspectiveControls() {}

  void setCursorPos(TPointD pos) { m_cursorPos = pos; }
  TPointD getCursorPos() { return m_cursorPos; }

  void setActive(bool active) { m_active = active; }
  bool isActive() { return m_active; }

  void setShowAdvancedControls(bool show) { m_showAdvanced = show; }
  bool showAdvancedControls() { return m_showAdvanced; }

  void setRotationPos(TPointD pos) { m_rotationPos = pos; }
  TPointD getRotationPos() { return m_rotationPos; }

  void setSpacingPos(TPointD pos) { m_spacingPos = pos; }
  TPointD getSpacingPos() { return m_spacingPos; }

  void setLeftPivotPos(TPointD pos) { m_leftPivotPos = pos; }
  TPointD getLeftPivotPos() { return m_leftPivotPos; }

  void setLeftHandlePos(TPointD pos) { m_leftHandlePos = pos; }
  TPointD getLeftHandlePos() { return m_leftHandlePos; }

  void setRightPivotPos(TPointD pos) { m_rightPivotPos = pos; }
  TPointD getRightPivotPos() { return m_rightPivotPos; }

  void setRightHandlePos(TPointD pos) { m_rightHandlePos = pos; }
  TPointD getRightHandlePos() { return m_rightHandlePos; }

  void shiftPerspectiveObject(TPointD delta);

  void drawControls(SceneViewer *viewer);

  bool isOverCenterPoint() {
    TPointD centerPoint = m_perspective->getCenterPoint();
    return checkOver(centerPoint, m_cursorPos);
  }
  bool isOverRotation() { return checkOver(m_rotationPos, m_cursorPos); }
  bool isOverSpacing() {
    if (!m_perspective || (m_perspective->getType() == PerspectiveType::Line &&
                           !m_perspective->isParallel()))
      return false;

    return checkOver(m_spacingPos, m_cursorPos);
  }

  bool isOverLeftPivot() {
    if (!m_showAdvanced || !m_perspective ||
        m_perspective->getType() != PerspectiveType::VanishingPoint)
      return false;

    return checkOver(m_leftPivotPos, m_cursorPos);
  }

  bool isOverLeftHandle() {
    if (!m_showAdvanced || !m_perspective ||
        m_perspective->getType() != PerspectiveType::VanishingPoint)
      return false;

    return checkOver(m_leftHandlePos, m_cursorPos);
  }

  bool isOverRightPivot() {
    if (!m_showAdvanced || !m_perspective ||
        m_perspective->getType() != PerspectiveType::VanishingPoint)
      return false;

    return checkOver(m_rightPivotPos, m_cursorPos);
  }

  bool isOverRightHandle() {
    if (!m_showAdvanced || !m_perspective ||
        m_perspective->getType() != PerspectiveType::VanishingPoint)
      return false;

    return checkOver(m_rightHandlePos, m_cursorPos);
  }

  bool isOverControl() {
    return isOverCenterPoint() | isOverRotation() | isOverSpacing() |
           isOverLeftPivot() | isOverLeftHandle() | isOverRightPivot() |
           isOverRightHandle();
  }

  TRectD getCenterPointRect() {
    TPointD centerPoint = m_perspective->getCenterPoint();
    return getControlRect(centerPoint);
  }
  TRectD getRotationRect() { return getControlRect(m_rotationPos); }
  TRectD getSpacingRect() { return getControlRect(m_spacingPos); }
  TRectD getLeftPivotRect() { return getControlRect(m_leftPivotPos); }
  TRectD getLeftHandleRect() { return getControlRect(m_leftHandlePos); }
  TRectD getRightPivotRect() { return getControlRect(m_rightPivotPos); }
  TRectD getRightHandleRect() { return getControlRect(m_rightHandlePos); }

private:
  TRectD getControlRect(TPointD controlPos) {
    return TRectD(controlPos.x - m_handleRadius * m_unit,
                  controlPos.y + m_handleRadius * m_unit,
                  controlPos.x + m_handleRadius * m_unit,
                  controlPos.y - m_handleRadius * m_unit);
  }

  bool checkOver(TPointD controlPos, TPointD mousePos) {
    return areAlmostEqual(controlPos.x, mousePos.x, m_handleRadius * m_unit) &&
           areAlmostEqual(controlPos.y, mousePos.y, m_handleRadius * m_unit);
  }
};

//=============================================================================
// PerspectiveObject
//-----------------------------------------------------------------------------

class PerspectiveObject : public PerspectiveData, public PerspectiveControls {
public:
  PerspectiveObject(PerspectiveType perspectiveType,
                    TPointD centerPoint = TPointD(0, 0), double rotation = 0.0,
                    double spacing = 100.0, bool horizon = false,
                    double opacity = 30.0, TPixel32 color = TPixel::Magenta,
                    bool parallel = true, bool showAdvanced = false)
      : PerspectiveData(perspectiveType, centerPoint, rotation, spacing,
                        horizon, opacity, color, parallel)
      , PerspectiveControls(this){};
  ~PerspectiveObject(){};

  virtual TPointD getReferencePoint(TPointD firstPoint) { return firstPoint; }
};

//=============================================================================
// PerspectiveObjectUndo
//-----------------------------------------------------------------------------

class PerspectiveObjectUndo final : public TUndo {
  std::vector<PerspectiveObject *> m_undoData;
  std::vector<PerspectiveObject *> m_redoData;
  PerspectiveTool *m_tool;

public:
  PerspectiveObjectUndo(std::vector<PerspectiveObject *> objs,
                        PerspectiveTool *tool);
  ~PerspectiveObjectUndo() {}

  void undo() const override;
  void redo() const override;

  void setRedoData(std::vector<PerspectiveObject *> objs);

  int getSize() const override { return sizeof(this); }

  QString getToolName() { return QString("Perspective Grid Tool"); }
  int getHistoryType() override { return HistoryType::PerspectiveGridTool; }
  QString getHistoryString() override { return getToolName(); }
};

//=============================================================================
// PerspectivePreset
//-----------------------------------------------------------------------------

struct PerspectivePreset final : public TPersist {
  PERSIST_DECLARATION(PerspectivePreset)

  std::wstring m_presetName;
  std::vector<PerspectiveObject *> m_perspectiveSet;
  TFilePath m_path;

  PerspectivePreset() {}
  PerspectivePreset(std::wstring presetName, TFilePath path = TFilePath())
      : m_presetName(presetName), m_path(path) {}
  PerspectivePreset(std::wstring presetName,
                    std::vector<PerspectiveObject *> perspectiveSet,
                    TFilePath path = TFilePath())
      : m_presetName(presetName)
      , m_perspectiveSet(perspectiveSet)
      , m_path(path) {}

  void setPath(TFilePath path) { m_path = path; }
  TFilePath getPath() { return m_path; }

  bool operator<(const PerspectivePreset &other) const {
    return m_presetName < other.m_presetName;
  }

  void saveData(TOStream &os) override;
  void loadData(TIStream &is) override;
};

//************************************************************************
//   Perspective Preset Manager declaration
//************************************************************************

class PerspectivePresetManager {
  std::set<PerspectivePreset> m_presets;  //!< Current presets container
  bool m_presetsLoaded = false;
  PerspectivePreset m_customPreset;

public:
  PerspectivePresetManager() {}

  void setIsLoaded(bool isLoaded) { m_presetsLoaded = isLoaded; }
  bool isLoaded() { return m_presetsLoaded; }

  void clearPresets() {
    m_presetsLoaded = false;
    m_presets.clear();
  }
  void loadPresets(const TFilePath &presetFolder);
  void savePreset(std::wstring presetName);
  void deletePreset(TFilePath presetPath);

  const std::set<PerspectivePreset> &presets() const { return m_presets; }

  PerspectivePreset getCustomPreset() { return m_customPreset; }
  void setCustomPreset(PerspectivePreset custom) { m_customPreset = custom; }

  void addPreset(PerspectivePreset perspectiveSet);
  void removePreset(const std::wstring &name);
};

//=============================================================================
// PerspectiveSelection
//-----------------------------------------------------------------------------

class PerspectiveSelection final : public TSelection {
private:
  std::set<int> m_selectedObjects;

  PerspectiveTool *m_tool;

public:
  PerspectiveSelection() : m_tool(0) {}

  void enableCommands() override;

  void setTool(PerspectiveTool *tool) { m_tool = tool; }

  bool isEmpty() const override { return m_selectedObjects.empty(); }

  void selectNone() override { m_selectedObjects.clear(); }

  bool isSelected(int index) {
    return m_selectedObjects.find(index) != m_selectedObjects.end();
  }
  void select(int index) { m_selectedObjects.insert(index); }
  void unselect(int index) { m_selectedObjects.erase(index); }

  int count() { return m_selectedObjects.size(); }
  std::set<int> getSelectedObjects() { return m_selectedObjects; }
};

//=============================================================================
// VanishingPointPerspective
//-----------------------------------------------------------------------------

class VanishingPointPerspective final : public PerspectiveObject {
public:
  VanishingPointPerspective()
      : PerspectiveObject(PerspectiveType::VanishingPoint) {
    setSpacing(27);
    setParallel(false);
    setMaxSpacing(180);
    setRotationPos(TPointD(0, -40));
    setSpacingPos(TPointD(20, -40));
  };
  ~VanishingPointPerspective(){};

  void draw(SceneViewer *viewer, TRectD cameraRect) override;

  double getMaxSpacing() override {
    if (isHorizon()) return 90.0;

    return m_maxSpacing;
  }

  TPointD getReferencePoint(TPointD firstPoint) override {
    return getCenterPoint();
  }
};

//=============================================================================
// LinePerspective
//-----------------------------------------------------------------------------

class LinePerspective final : public PerspectiveObject {
public:
  LinePerspective() : PerspectiveObject(PerspectiveType::Line) {
    setSpacing(40);
  };
  ~LinePerspective(){};

  void draw(SceneViewer *viewer, TRectD cameraRect) override;

  TPointD getReferencePoint(TPointD firstPoint) override;
};

//=============================================================================
// PerspectiveTool
//-----------------------------------------------------------------------------

class PerspectiveTool final : public TTool, public TProjectManager::Listener {
  Q_DECLARE_TR_FUNCTIONS(PerspectiveTool)

public:
  PerspectiveTool();
  ~PerspectiveTool();

  ToolType getToolType() const override { return TTool::GenericTool; }

  void updateTranslation();

  void draw(SceneViewer *viewer) override;

  void leftButtonDown(const TPointD &pos, const TMouseEvent &e) override;
  void leftButtonDrag(const TPointD &pos, const TMouseEvent &e) override;
  void leftButtonUp(const TPointD &pos, const TMouseEvent &) override;
  bool keyDown(QKeyEvent *event) override;

  void onActivate() override;
  bool onPropertyChanged(std::string propertyName) override;

  int getCursorId() const override { return ToolCursor::StrokeSelectCursor; };

  TPropertyGroup *getProperties(int targetType) override;

  void setToolOptionsBox(PerspectiveGridToolOptionBox *toolOptionsBox);

  void deleteSelectedObjects();
  void selectAllObjects();

  void setPerspectiveObjects(std::vector<PerspectiveObject *> objs);
  std::vector<PerspectiveObject *> getPerspectiveObjects() {
    if (!m_guideEnabled) return std::vector<PerspectiveObject *>();

    return m_perspectiveObjs;
  }

  PerspectiveObject *getMainPerspectiveObject() {
    if (m_mainControlIndex < 0 ||
        m_mainControlIndex >= m_perspectiveObjs.size())
      return 0;

    return m_perspectiveObjs[m_mainControlIndex];
  }

  std::vector<PerspectiveObject *> copyPerspectiveSet(
      std::vector<PerspectiveObject *> perspectiveSet, bool keepStatus = true);

  void invalidateControl(int controlIndex);

  void onEnter() override;
  void onDeactivate() override;

  void setGuideEnabled(bool enabled) { m_guideEnabled = enabled; }
  void toggleGuideEnabled() { m_guideEnabled = !m_guideEnabled; }
  bool isGuideEnabled() { return m_guideEnabled; }

  void initPresets();
  void loadPreset();
  void addPreset(QString name, bool isLibrary = false);
  void removePreset();
  void loadLastPreset();

  void updateSpacing(double space);
  void updateRotation(double rotation);

  void updateMeasuredValueToolOptions();
  void updateToolOptionValues();

  void saveTool() override;
  void loadTool() override;

  /*!	Overridden from TProjectManager::Listener. */
  void onProjectSwitched() override;
  void onProjectChanged() override;

  // returns true if the pressed key is recognized and processed.
  bool isEventAcceptable(QEvent *e) override;

protected:
  TPropertyGroup m_prop;
  std::vector<PerspectiveGridToolOptionBox *> m_toolOptionsBox;

  TEnumProperty m_type;
  TDoubleProperty m_opacity;
  TColorChipProperty m_color;
  TBoolProperty m_horizon;
  TBoolProperty m_parallel;
  TEnumProperty m_preset;
  TBoolProperty m_advancedControls;

  std::vector<PerspectiveObject *> m_perspectiveObjs;
  std::vector<PerspectiveObject *> m_lastPreset;

  bool m_firstTime;

  PerspectiveSelection m_selection;
  bool m_selecting;
  TRectD m_selectingRect;

  bool m_modified;
  bool m_isShifting;
  bool m_isCenterMoving;
  bool m_isRotating;
  bool m_isSpacing;
  bool m_isLeftPivoting;
  bool m_isLeftMoving;
  bool m_isRightPivoting;
  bool m_isRightMoving;

  TPointD m_mousePos, m_firstPos;

  double m_totalChange;

  int m_mainControlIndex;

  bool m_propertyUpdating = false;

  PerspectivePresetManager m_presetsManager;

  PerspectiveObjectUndo *m_undo;

  bool m_guideEnabled;
};

#endif