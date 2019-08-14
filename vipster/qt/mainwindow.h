#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAbstractButton>
#include <QDir>
#include <QTimer>
#include <vector>
#include "io.h"
#include "stepsel.h"
#include "configfile.h"
#include "../common/guiwrapper.h"
#include "paramwidget.h"
#include "configwidget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QString path, Vipster::ConfigState& state,
                        std::vector<Vipster::IO::Data> &&d={},
                        QWidget *parent = nullptr);
    ~MainWindow() override;
    Vipster::Molecule* curMol{nullptr};
    Vipster::Step* curStep{nullptr};
    Vipster::Step::selection* curSel{nullptr};
    Vipster::Step copyBuf{};
    void updateWidgets(Vipster::guiChange_t change);
    void newData(Vipster::IO::Data&& d);
    void setMultEnabled(bool);
    struct MolExtras{
        int curStep{-1};
        Vipster::PBCVec mult{1,1,1};
    };
    struct StepExtras{
        std::unique_ptr<Vipster::Step::selection> sel{nullptr};
        std::map<std::string, Vipster::Step::selection> def{};
    };
    std::list<Vipster::Molecule> molecules;
    std::map<Vipster::Molecule*, MolExtras> moldata;
    std::map<Vipster::Step*, StepExtras> stepdata;
    std::list<std::unique_ptr<const Vipster::BaseData>> data;
    // expose configstate read from file
    Vipster::ConfigState    &state;
    Vipster::PeriodicTable  &pte;
    Vipster::Settings       &settings;
    Vipster::IO::Parameters &params;
    Vipster::IO::Configs    &configs;
    // actually loaded and sortet params/configs
    const decltype (ParamWidget::params)& getParams() const noexcept;
    const decltype (ConfigWidget::configs)& getConfigs() const noexcept;
    // GL helpers for additional render-data
    void addExtraData(Vipster::GUI::Data* dat);
    void delExtraData(Vipster::GUI::Data* dat);
    const Vipster::GUI::GlobalData& getGLGlobals();

public slots:
    void setMol(int i);
    void setStep(int i);
    void setMult(int i);
    void stepBut(QAbstractButton *but);
    void about();
    void newMol();
    void newMol(QAction *sender);
    void loadMol();
    void saveMol();
    void editAtoms(QAction *sender);
    void loadParam();
    void saveParam();
    void loadConfig();
    void saveConfig();
    void saveScreenshot();

private:
    void setupUI(void);

    Ui::MainWindow *ui;
    QDir path{};
    QTimer playTimer{};
    std::vector<QDockWidget*> baseWidgets;
    std::vector<QDockWidget*> toolWidgets;
};
#endif // MAINWINDOW_H
