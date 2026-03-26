#pragma once

#include <QMainWindow>

class QCheckBox;
class QComboBox;
class QDialog;
class QGroupBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QRadioButton;
class QScrollArea;
class QStackedWidget;
class QTabWidget;
class QTableWidget;
class QTimer;
class QTreeWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(quint16 bridgePort, QWidget* parent = nullptr);

private:
    void buildUi();
    void wireSignals();
    void appendUiLog(const QString& line);
    void refreshBridgeLog();
    void refreshOptionsSummary();
    void refreshPageSummary();
    void showConfirmDialog();

    quint16 m_bridgePort = 0;
    int m_counter = 0;
    QString m_lastBridgeLogSnapshot;

    QLabel* m_statusLabel = nullptr;
    QLabel* m_counterLabel = nullptr;
    QLabel* m_bridgeLabel = nullptr;
    QLabel* m_usersStatusLabel = nullptr;
    QLabel* m_dialogResultLabel = nullptr;
    QLabel* m_secretStatusLabel = nullptr;
    QLabel* m_optionsSummaryLabel = nullptr;
    QLabel* m_currentPageLabel = nullptr;

    QLineEdit* m_usernameEdit = nullptr;
    QLineEdit* m_passwordEdit = nullptr;

    QPushButton* m_loginButton = nullptr;
    QPushButton* m_incrementButton = nullptr;
    QPushButton* m_resetButton = nullptr;
    QPushButton* m_loadUsersButton = nullptr;
    QPushButton* m_revealSecretButton = nullptr;
    QPushButton* m_secretActionButton = nullptr;
    QPushButton* m_openConfirmDialogButton = nullptr;
    QPushButton* m_showFirstPageButton = nullptr;
    QPushButton* m_showSecondPageButton = nullptr;
    QPushButton* m_scrollTargetButton = nullptr;

    QProgressBar* m_usersProgressBar = nullptr;
    QListWidget* m_usersListWidget = nullptr;
    QGroupBox* m_secretPanel = nullptr;
    QPlainTextEdit* m_uiLogView = nullptr;
    QPlainTextEdit* m_bridgeLogView = nullptr;
    QTimer* m_logRefreshTimer = nullptr;

    QCheckBox* m_enableAdvancedCheck = nullptr;
    QRadioButton* m_modeStandardRadio = nullptr;
    QRadioButton* m_modeExpertRadio = nullptr;
    QComboBox* m_themeCombo = nullptr;

    QTabWidget* m_demoTabWidget = nullptr;
    QTreeWidget* m_demoTreeWidget = nullptr;
    QTableWidget* m_demoTableWidget = nullptr;
    QScrollArea* m_demoScrollArea = nullptr;
    QStackedWidget* m_pageStackedWidget = nullptr;
};
