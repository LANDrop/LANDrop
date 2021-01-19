#include <QFileDialog>

#include "settings.h"
#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    connect(ui->downloadPathSelectButton, &QToolButton::clicked, this, &SettingsDialog::downloadPathSelectButtonClicked);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::accept()
{
    Settings::setMachineName(ui->machineNameLineEdit->text());
    Settings::setDownloadPath(ui->downloadPathLineEdit->text());
    done(Accepted);
}

void SettingsDialog::downloadPathSelectButtonClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Download Path"),
                                                    ui->downloadPathLineEdit->text());
    if (!dir.isEmpty())
        ui->downloadPathLineEdit->setText(dir);
}

void SettingsDialog::showEvent(QShowEvent *e)
{
    QDialog::showEvent(e);
    ui->machineNameLineEdit->setText(Settings::machineName());
    ui->downloadPathLineEdit->setText(Settings::downloadPath());
    ui->machineNameLineEdit->setFocus();
}
