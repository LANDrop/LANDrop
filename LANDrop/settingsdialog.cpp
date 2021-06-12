/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2021, LANDrop
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdexcept>

#include <QDesktopServices>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>
#include <QVersionNumber>

#include "settings.h"
#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowStaysOnTopHint);
    connect(ui->downloadPathSelectButton, &QToolButton::clicked, this, &SettingsDialog::downloadPathSelectButtonClicked);
    connect(ui->serverPortLineEdit, &QLineEdit::textChanged, this, &SettingsDialog::serverPortLineEditChanged);
    connect(ui->checkForUpdatesButton, &QPushButton::clicked, this, &SettingsDialog::checkForUpdatesButtonClicked);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::accept()
{
    Settings::setDeviceName(ui->deviceNameLineEdit->text());
    Settings::setDownloadPath(ui->downloadPathLineEdit->text());
    Settings::setDiscoverable(ui->discoverableCheckBox->isChecked());
    Settings::setServerPort(ui->serverPortLineEdit->text().toUShort());
    if (serverPortEdited)
        QMessageBox::information(this, QApplication::applicationName(),
                                 tr("Server port setting will take effect after you restart the app."));
    done(Accepted);
}

void SettingsDialog::downloadPathSelectButtonClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Download Path"),
                                                    ui->downloadPathLineEdit->text());
    if (!dir.isEmpty())
        ui->downloadPathLineEdit->setText(dir);
}

void SettingsDialog::serverPortLineEditChanged()
{
    serverPortEdited = true;
}

void SettingsDialog::checkForUpdatesButtonClicked()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, [&, manager](QNetworkReply *reply) {
        try {
            if (reply->error())
                throw std::runtime_error(reply->errorString().toUtf8().toStdString());

            QByteArray body = reply->readAll();
            QJsonDocument json = QJsonDocument::fromJson(body);
            if (!json.isObject())
                throw std::runtime_error(tr("Failed to get latest version.").toUtf8().toStdString());

            QJsonObject obj = json.object();
            QJsonValue verJson = obj.value("desktop");
            if (!verJson.isString())
                throw std::runtime_error(tr("Failed to get latest version.").toUtf8().toStdString());

            QString version = verJson.toString();
            QVersionNumber curVersion = QVersionNumber::fromString(QApplication::applicationVersion());
            QVersionNumber latestVersion = QVersionNumber::fromString(version);
            if (latestVersion > curVersion) {
                if (QMessageBox::question(this, QApplication::applicationName(),
                                          tr("There is a new version %1! Do you want to update?").arg(version))
                        == QMessageBox::Yes) {
                    QDesktopServices::openUrl(QUrl::fromEncoded("https://landrop.app/#downloads"));
                }
            } else {
                QMessageBox::information(this, QApplication::applicationName(),
                                         tr("You have the latest version!"));
            }
        } catch (const std::exception &e) {
            QMessageBox::critical(this, QApplication::applicationName(), e.what());
        }
        manager->deleteLater();
        ui->checkForUpdatesButton->setEnabled(true);
    });
    QNetworkRequest request(QUrl("https://releases.landrop.app/versions.json"));
    ui->checkForUpdatesButton->setEnabled(false);
    manager->get(request);
}

void SettingsDialog::showEvent(QShowEvent *e)
{
    QDialog::showEvent(e);
    ui->deviceNameLineEdit->setText(Settings::deviceName());
    ui->downloadPathLineEdit->setText(Settings::downloadPath());
    ui->discoverableCheckBox->setChecked(Settings::discoverable());
    ui->serverPortLineEdit->setText(QString::number(Settings::serverPort()));
    ui->deviceNameLineEdit->setFocus();
    serverPortEdited = false;
}
