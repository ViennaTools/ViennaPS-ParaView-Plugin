#ifndef pqViennaPSWidget_h
#define pqViennaPSWidget_h

#include <pqPropertyWidget.h>
#include <QWidget>
#include <QMap>
#include <memory>

class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;
class QGroupBox;
class QFormLayout;
class QVBoxLayout;
class QLabel;

class vtkSMProxy;
class vtkSMProperty;
class vtkSMPropertyGroup;

struct ParameterMetadata;

class pqViennaPSWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqViennaPSWidget(vtkSMProxy* proxy,
                   QWidget* parent = nullptr);
  pqViennaPSWidget(vtkSMProxy* proxy, vtkSMProperty* property, 
                   QWidget* parent = nullptr);

  ~pqViennaPSWidget() override;

  void apply() override;

  static pqPropertyWidget* createWidget(vtkSMProxy* proxy,
                                        vtkSMProperty* property,
                                        QWidget* parent);

  static pqPropertyWidget* createWidget(vtkSMProxy* proxy,
                                       QWidget* parent);

private slots:
  void onModelChanged(int index);

  void onDoubleParameterChanged(double value);
  void onIntParameterChanged(int value);
  void onBoolParameterChanged(bool value);
  void onEnumParameterChanged(int index);

  void updateParameterVisibility();

  void updateDomainSettingsVisibility();

  void applyChanges();

private:
  bool isSource = true;
  void initialize();
  void determineWidgetType();
  void createParameterWidgets();
  void clearParameterWidgets();

  QWidget* createDoubleWidget(const QString& name, double value,
                              double min, double max, const QString& suffix,
                              double stepSize = 0.1);
  QWidget* createIntWidget(const QString& name, int value,
                          int min, int max);
  QWidget* createBoolWidget(const QString& name, bool value);
  QWidget* createEnumWidget(const QString& name,
                           const QStringList& options, int value);

  QWidget* createMaterialListWidget(const QString& name,
                                 const QStringList& options);

  void loadModel(const QString& modelName);

  bool evaluateCondition(const QString& condition);

  bool hasInputDomainInfo();

  QVBoxLayout* mainLayout;
  QComboBox* modelSelector;
  QGroupBox* commonParamsGroup;
  QGroupBox* modelParamsGroup;
  QFormLayout* commonForm;
  QFormLayout* modelForm;
  QGroupBox* commonProcessGroup;
  QFormLayout* commonProcessForm;

  QMap<QString, QWidget*> parameterWidgets;
  QMap<QString, QLabel*> parameterLabels;
  QMap<QString, QVariant> parameterValues;

  QDoubleSpinBox* gridDeltaSpinBox;
  QDoubleSpinBox* xExtentSpinBox;
  QDoubleSpinBox* yExtentSpinBox;
  QComboBox* targetDimensionComboBox;
  QDoubleSpinBox* processTimeSpinBox;
  QSpinBox* numRaysSpinBox;
  QComboBox* outputFormatComboBox;

  QString currentModel;
  vtkSMProxy* proxy;
};

#endif
