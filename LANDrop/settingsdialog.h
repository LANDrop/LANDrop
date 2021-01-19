#pragma once

#include <QDialog>

namespace Ui {
    class SettingsDialog;
}

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();
private:
    Ui::SettingsDialog *ui;
private slots:
    void accept();
    void downloadPathSelectButtonClicked();
protected:
    void showEvent(QShowEvent *e);
};
