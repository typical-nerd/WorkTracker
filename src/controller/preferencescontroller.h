/*
 * Copyright 2014 Robert Lohr
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SETTINGSCONTROLLER_H
#define SETTINGSCONTROLLER_H

#include <QObject>

class PreferencesDialog;
class QString;

/*!
 * Manages the actions of the `SettingsDialog`.
 */
class PreferencesController : public QObject
{
    Q_OBJECT
public:
    /*!
     * Creates a new settings controller and dialog. The ui is shown by calling
     * `SettingsController::exec()`.
     * 
     * \param p_parentWidget
     * The parent widget of the dialog.
     * 
     * \param p_parent
     * The controller's parent.
     */
    PreferencesController(QWidget* p_parentWidget, QObject* p_parent = nullptr);
    
    /*!
     * Shows a modal settings dialog but does not block.
     */
    void run();
    
signals:
    /*!
     * Emitted when the dialog is closed.
     * 
     * \param p_saved
     * `true` if the settings dialog was closed due to a click on the "Save" button or
     * `false` if all changes have been discarded.
     */
    void finished(bool p_saved);

    /*!
     * Emitted when a different language has been selected in the ui.
     *
     * \param p_locale
     * The locale of the selected language.
     */
    void languageChanged(const QString& p_locale);
    
public slots:
    /*!
     * Saves the settings.
     */
    void saveSettings();
    
    /*!
     * Restore the settings to the state before opening the dialog. This only applies to
     * settings that take immediate effect, e.g. changing the language.
     */
    void restoreSettings();
    
private:
    PreferencesDialog* m_dialog;
    
};

#endif // SETTINGSCONTROLLER_H
