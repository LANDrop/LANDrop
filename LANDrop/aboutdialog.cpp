#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent), ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->aboutText->setHtml(
                ui->aboutText->toHtml().arg(QApplication::applicationName(),
                                            QApplication::applicationVersion(),
                                            QApplication::organizationName()));
    connect(ui->aboutQtButton, &QPushButton::clicked, this, &AboutDialog::aboutQtButtonClicked);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::aboutQtButtonClicked()
{
    QApplication::aboutQt();
}
