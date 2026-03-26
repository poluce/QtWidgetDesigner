#include "main_window.h"

#include "app_log_sink.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(quint16 bridgePort, QWidget* parent)
    : QMainWindow(parent)
    , m_bridgePort(bridgePort)
{
    buildUi();
    wireSignals();
}

void MainWindow::buildUi()
{
    setObjectName(QStringLiteral("mainWindow"));
    setWindowTitle(QStringLiteral("Qt 智能自动测试演示台"));
    resize(1180, 860);

    auto* central = new QWidget(this);
    central->setObjectName(QStringLiteral("centralPage"));
    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(18, 18, 18, 18);
    rootLayout->setSpacing(14);

    auto* introLabel = new QLabel(
        QStringLiteral("这是一个面向外部 LLM 工具调用的 Qt Widgets 自动化演示界面。你可以观察对象树、布局树、输入、点击、等待与断言在真实界面上的表现。"),
        central);
    introLabel->setObjectName(QStringLiteral("introLabel"));
    introLabel->setWordWrap(true);
    rootLayout->addWidget(introLabel);

    auto* bridgeBox = new QGroupBox(QStringLiteral("桥接服务"), central);
    bridgeBox->setObjectName(QStringLiteral("bridgeBox"));
    auto* bridgeLayout = new QVBoxLayout(bridgeBox);
    m_bridgeLabel = new QLabel(
        QStringLiteral("WebSocket 地址：ws://127.0.0.1:%1").arg(m_bridgePort), bridgeBox);
    m_bridgeLabel->setObjectName(QStringLiteral("bridgeStatusLabel"));
    bridgeLayout->addWidget(m_bridgeLabel);
    rootLayout->addWidget(bridgeBox);

    m_demoTabWidget = new QTabWidget(central);
    m_demoTabWidget->setObjectName(QStringLiteral("demoTabWidget"));

    auto* basicPage = new QWidget(m_demoTabWidget);
    basicPage->setObjectName(QStringLiteral("basicPage"));
    auto* basicLayout = new QVBoxLayout(basicPage);
    basicLayout->setSpacing(12);

    auto* loginBox = new QGroupBox(QStringLiteral("登录表单"), basicPage);
    loginBox->setObjectName(QStringLiteral("loginBox"));
    auto* loginLayout = new QFormLayout(loginBox);
    m_usernameEdit = new QLineEdit(loginBox);
    m_usernameEdit->setObjectName(QStringLiteral("usernameEdit"));
    m_usernameEdit->setPlaceholderText(QStringLiteral("请输入用户名"));
    loginLayout->addRow(QStringLiteral("用户名"), m_usernameEdit);
    m_passwordEdit = new QLineEdit(loginBox);
    m_passwordEdit->setObjectName(QStringLiteral("passwordEdit"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(QStringLiteral("请输入密码"));
    loginLayout->addRow(QStringLiteral("密码"), m_passwordEdit);
    m_loginButton = new QPushButton(QStringLiteral("登录"), loginBox);
    m_loginButton->setObjectName(QStringLiteral("loginButton"));
    loginLayout->addRow(QString(), m_loginButton);
    basicLayout->addWidget(loginBox);

    auto* optionsBox = new QGroupBox(QStringLiteral("选项面板"), basicPage);
    optionsBox->setObjectName(QStringLiteral("optionsBox"));
    auto* optionsLayout = new QGridLayout(optionsBox);
    m_enableAdvancedCheck = new QCheckBox(QStringLiteral("启用高级模式"), optionsBox);
    m_enableAdvancedCheck->setObjectName(QStringLiteral("enableAdvancedCheck"));
    optionsLayout->addWidget(m_enableAdvancedCheck, 0, 0, 1, 2);
    m_modeStandardRadio = new QRadioButton(QStringLiteral("标准模式"), optionsBox);
    m_modeStandardRadio->setObjectName(QStringLiteral("modeStandardRadio"));
    m_modeExpertRadio = new QRadioButton(QStringLiteral("专家模式"), optionsBox);
    m_modeExpertRadio->setObjectName(QStringLiteral("modeExpertRadio"));
    m_modeStandardRadio->setChecked(true);
    optionsLayout->addWidget(m_modeStandardRadio, 1, 0);
    optionsLayout->addWidget(m_modeExpertRadio, 1, 1);
    auto* themeLabel = new QLabel(QStringLiteral("主题"), optionsBox);
    themeLabel->setObjectName(QStringLiteral("themeLabel"));
    optionsLayout->addWidget(themeLabel, 2, 0);
    m_themeCombo = new QComboBox(optionsBox);
    m_themeCombo->setObjectName(QStringLiteral("themeCombo"));
    m_themeCombo->addItems(QStringList{
        QStringLiteral("浅色"),
        QStringLiteral("深色"),
        QStringLiteral("高对比"),
    });
    optionsLayout->addWidget(m_themeCombo, 2, 1);
    m_optionsSummaryLabel = new QLabel(QStringLiteral("当前选项：标准模式 / 浅色 / 高级模式关闭"), optionsBox);
    m_optionsSummaryLabel->setObjectName(QStringLiteral("optionsSummaryLabel"));
    optionsLayout->addWidget(m_optionsSummaryLabel, 3, 0, 1, 2);
    basicLayout->addWidget(optionsBox);

    auto* counterBox = new QGroupBox(QStringLiteral("计数器操作"), basicPage);
    counterBox->setObjectName(QStringLiteral("counterBox"));
    auto* counterLayout = new QHBoxLayout(counterBox);
    m_counterLabel = new QLabel(QStringLiteral("计数器：0"), counterBox);
    m_counterLabel->setObjectName(QStringLiteral("counterLabel"));
    counterLayout->addWidget(m_counterLabel, 1);
    m_incrementButton = new QPushButton(QStringLiteral("增加"), counterBox);
    m_incrementButton->setObjectName(QStringLiteral("incrementButton"));
    counterLayout->addWidget(m_incrementButton);
    m_resetButton = new QPushButton(QStringLiteral("重置"), counterBox);
    m_resetButton->setObjectName(QStringLiteral("resetButton"));
    counterLayout->addWidget(m_resetButton);
    basicLayout->addWidget(counterBox);

    auto* dialogBox = new QGroupBox(QStringLiteral("对话框演示"), basicPage);
    dialogBox->setObjectName(QStringLiteral("dialogBox"));
    auto* dialogLayout = new QVBoxLayout(dialogBox);
    m_openConfirmDialogButton = new QPushButton(QStringLiteral("打开确认对话框"), dialogBox);
    m_openConfirmDialogButton->setObjectName(QStringLiteral("openConfirmDialogButton"));
    dialogLayout->addWidget(m_openConfirmDialogButton);
    m_dialogResultLabel = new QLabel(QStringLiteral("对话框结果：无"), dialogBox);
    m_dialogResultLabel->setObjectName(QStringLiteral("dialogResultLabel"));
    dialogLayout->addWidget(m_dialogResultLabel);
    basicLayout->addWidget(dialogBox);
    basicLayout->addStretch(1);

    auto* dataPage = new QWidget(m_demoTabWidget);
    dataPage->setObjectName(QStringLiteral("dataPage"));
    auto* dataLayout = new QVBoxLayout(dataPage);
    dataLayout->setSpacing(12);

    auto* asyncBox = new QGroupBox(QStringLiteral("异步数据演示"), dataPage);
    asyncBox->setObjectName(QStringLiteral("asyncBox"));
    auto* asyncLayout = new QVBoxLayout(asyncBox);
    auto* asyncButtonRow = new QHBoxLayout();
    m_loadUsersButton = new QPushButton(QStringLiteral("加载演示用户"), asyncBox);
    m_loadUsersButton->setObjectName(QStringLiteral("loadUsersButton"));
    asyncButtonRow->addWidget(m_loadUsersButton);
    m_revealSecretButton = new QPushButton(QStringLiteral("显示隐藏面板"), asyncBox);
    m_revealSecretButton->setObjectName(QStringLiteral("revealSecretButton"));
    asyncButtonRow->addWidget(m_revealSecretButton);
    asyncLayout->addLayout(asyncButtonRow);
    m_usersProgressBar = new QProgressBar(asyncBox);
    m_usersProgressBar->setObjectName(QStringLiteral("usersProgressBar"));
    m_usersProgressBar->setRange(0, 100);
    m_usersProgressBar->setValue(0);
    asyncLayout->addWidget(m_usersProgressBar);
    m_usersStatusLabel = new QLabel(QStringLiteral("用户加载状态：0"), asyncBox);
    m_usersStatusLabel->setObjectName(QStringLiteral("usersStatusLabel"));
    asyncLayout->addWidget(m_usersStatusLabel);
    m_usersListWidget = new QListWidget(asyncBox);
    m_usersListWidget->setObjectName(QStringLiteral("usersListWidget"));
    asyncLayout->addWidget(m_usersListWidget);
    m_secretPanel = new QGroupBox(QStringLiteral("隐藏面板"), asyncBox);
    m_secretPanel->setObjectName(QStringLiteral("secretPanel"));
    m_secretPanel->setVisible(false);
    auto* secretLayout = new QVBoxLayout(m_secretPanel);
    m_secretStatusLabel = new QLabel(QStringLiteral("隐藏面板当前不可见。"), m_secretPanel);
    m_secretStatusLabel->setObjectName(QStringLiteral("secretStatusLabel"));
    secretLayout->addWidget(m_secretStatusLabel);
    m_secretActionButton = new QPushButton(QStringLiteral("确认隐藏操作"), m_secretPanel);
    m_secretActionButton->setObjectName(QStringLiteral("secretActionButton"));
    secretLayout->addWidget(m_secretActionButton);
    asyncLayout->addWidget(m_secretPanel);
    dataLayout->addWidget(asyncBox);

    auto* structureBox = new QGroupBox(QStringLiteral("树形与表格"), dataPage);
    structureBox->setObjectName(QStringLiteral("structureBox"));
    auto* structureLayout = new QHBoxLayout(structureBox);
    m_demoTreeWidget = new QTreeWidget(structureBox);
    m_demoTreeWidget->setObjectName(QStringLiteral("demoTreeWidget"));
    m_demoTreeWidget->setHeaderLabels(QStringList{QStringLiteral("节点"), QStringLiteral("说明")});
    auto* settingsItem = new QTreeWidgetItem(QStringList{QStringLiteral("设置"), QStringLiteral("根节点")});
    auto* accountItem = new QTreeWidgetItem(QStringList{QStringLiteral("账户"), QStringLiteral("账户设置")});
    auto* securityItem = new QTreeWidgetItem(QStringList{QStringLiteral("安全"), QStringLiteral("安全选项")});
    auto* notificationItem = new QTreeWidgetItem(QStringList{QStringLiteral("通知"), QStringLiteral("通知选项")});
    settingsItem->addChild(accountItem);
    settingsItem->addChild(securityItem);
    settingsItem->addChild(notificationItem);
    auto* systemItem = new QTreeWidgetItem(QStringList{QStringLiteral("系统"), QStringLiteral("系统设置")});
    auto* updateItem = new QTreeWidgetItem(QStringList{QStringLiteral("更新"), QStringLiteral("更新策略")});
    systemItem->addChild(updateItem);
    m_demoTreeWidget->addTopLevelItem(settingsItem);
    m_demoTreeWidget->addTopLevelItem(systemItem);
    m_demoTreeWidget->expandItem(settingsItem);
    structureLayout->addWidget(m_demoTreeWidget, 1);

    m_demoTableWidget = new QTableWidget(3, 3, structureBox);
    m_demoTableWidget->setObjectName(QStringLiteral("demoTableWidget"));
    m_demoTableWidget->setHorizontalHeaderLabels(QStringList{
        QStringLiteral("姓名"),
        QStringLiteral("角色"),
        QStringLiteral("状态"),
    });
    m_demoTableWidget->verticalHeader()->setVisible(false);
    m_demoTableWidget->horizontalHeader()->setStretchLastSection(true);
    m_demoTableWidget->setItem(0, 0, new QTableWidgetItem(QStringLiteral("张三")));
    m_demoTableWidget->setItem(0, 1, new QTableWidgetItem(QStringLiteral("管理员")));
    m_demoTableWidget->setItem(0, 2, new QTableWidgetItem(QStringLiteral("在线")));
    m_demoTableWidget->setItem(1, 0, new QTableWidgetItem(QStringLiteral("李四")));
    m_demoTableWidget->setItem(1, 1, new QTableWidgetItem(QStringLiteral("审核员")));
    m_demoTableWidget->setItem(1, 2, new QTableWidgetItem(QStringLiteral("离线")));
    m_demoTableWidget->setItem(2, 0, new QTableWidgetItem(QStringLiteral("王五")));
    m_demoTableWidget->setItem(2, 1, new QTableWidgetItem(QStringLiteral("访客")));
    m_demoTableWidget->setItem(2, 2, new QTableWidgetItem(QStringLiteral("在线")));
    structureLayout->addWidget(m_demoTableWidget, 1);
    dataLayout->addWidget(structureBox);

    auto* pagePage = new QWidget(m_demoTabWidget);
    pagePage->setObjectName(QStringLiteral("pagePage"));
    auto* pageLayout = new QVBoxLayout(pagePage);
    pageLayout->setSpacing(12);

    auto* pageSwitchBox = new QGroupBox(QStringLiteral("多页面切换"), pagePage);
    pageSwitchBox->setObjectName(QStringLiteral("pageSwitchBox"));
    auto* pageSwitchLayout = new QVBoxLayout(pageSwitchBox);
    auto* pageButtonRow = new QHBoxLayout();
    m_showFirstPageButton = new QPushButton(QStringLiteral("切到第一页"), pageSwitchBox);
    m_showFirstPageButton->setObjectName(QStringLiteral("showFirstPageButton"));
    pageButtonRow->addWidget(m_showFirstPageButton);
    m_showSecondPageButton = new QPushButton(QStringLiteral("切到第二页"), pageSwitchBox);
    m_showSecondPageButton->setObjectName(QStringLiteral("showSecondPageButton"));
    pageButtonRow->addWidget(m_showSecondPageButton);
    pageSwitchLayout->addLayout(pageButtonRow);
    m_currentPageLabel = new QLabel(QStringLiteral("当前页面：第一页"), pageSwitchBox);
    m_currentPageLabel->setObjectName(QStringLiteral("currentPageLabel"));
    pageSwitchLayout->addWidget(m_currentPageLabel);
    m_pageStackedWidget = new QStackedWidget(pageSwitchBox);
    m_pageStackedWidget->setObjectName(QStringLiteral("pageStackedWidget"));
    auto* firstPage = new QWidget(m_pageStackedWidget);
    firstPage->setObjectName(QStringLiteral("stackPageOne"));
    auto* firstLayout = new QVBoxLayout(firstPage);
    firstLayout->addWidget(new QLabel(QStringLiteral("这里是第一页内容。"), firstPage));
    auto* secondPage = new QWidget(m_pageStackedWidget);
    secondPage->setObjectName(QStringLiteral("stackPageTwo"));
    auto* secondLayout = new QVBoxLayout(secondPage);
    secondLayout->addWidget(new QLabel(QStringLiteral("这里是第二页内容。"), secondPage));
    m_pageStackedWidget->addWidget(firstPage);
    m_pageStackedWidget->addWidget(secondPage);
    pageSwitchLayout->addWidget(m_pageStackedWidget);
    pageLayout->addWidget(pageSwitchBox);

    auto* scrollBox = new QGroupBox(QStringLiteral("滚动区域"), pagePage);
    scrollBox->setObjectName(QStringLiteral("scrollBox"));
    auto* scrollBoxLayout = new QVBoxLayout(scrollBox);
    m_demoScrollArea = new QScrollArea(scrollBox);
    m_demoScrollArea->setObjectName(QStringLiteral("demoScrollArea"));
    m_demoScrollArea->setWidgetResizable(true);
    auto* scrollContent = new QWidget(m_demoScrollArea);
    scrollContent->setObjectName(QStringLiteral("scrollContentWidget"));
    auto* scrollContentLayout = new QVBoxLayout(scrollContent);
    for (int i = 1; i <= 10; ++i) {
        auto* label = new QLabel(QStringLiteral("滚动内容段落 %1").arg(i), scrollContent);
        label->setObjectName(QStringLiteral("scrollLabel%1").arg(i));
        scrollContentLayout->addWidget(label);
    }
    m_scrollTargetButton = new QPushButton(QStringLiteral("滚动到底部后点击我"), scrollContent);
    m_scrollTargetButton->setObjectName(QStringLiteral("scrollTargetButton"));
    scrollContentLayout->addWidget(m_scrollTargetButton);
    scrollContentLayout->addStretch(1);
    m_demoScrollArea->setWidget(scrollContent);
    scrollBoxLayout->addWidget(m_demoScrollArea);
    pageLayout->addWidget(scrollBox);
    pageLayout->addStretch(1);

    m_demoTabWidget->addTab(basicPage, QStringLiteral("基础流程"));
    m_demoTabWidget->addTab(dataPage, QStringLiteral("数据结构"));
    m_demoTabWidget->addTab(pagePage, QStringLiteral("页面与滚动"));
    rootLayout->addWidget(m_demoTabWidget, 1);

    m_statusLabel = new QLabel(QStringLiteral("状态：空闲"), central);
    m_statusLabel->setObjectName(QStringLiteral("statusLabel"));
    rootLayout->addWidget(m_statusLabel);

    auto* logBox = new QGroupBox(QStringLiteral("演示活动"), central);
    logBox->setObjectName(QStringLiteral("logBox"));
    auto* logLayout = new QHBoxLayout(logBox);
    m_uiLogView = new QPlainTextEdit(logBox);
    m_uiLogView->setObjectName(QStringLiteral("eventLogView"));
    m_uiLogView->setReadOnly(true);
    m_uiLogView->setPlaceholderText(QStringLiteral("这里显示更适合人类观察的演示事件。"));
    logLayout->addWidget(m_uiLogView, 1);
    m_bridgeLogView = new QPlainTextEdit(logBox);
    m_bridgeLogView->setObjectName(QStringLiteral("bridgeLogView"));
    m_bridgeLogView->setReadOnly(true);
    m_bridgeLogView->setPlaceholderText(QStringLiteral("这里实时显示桥接命令和应用日志。"));
    logLayout->addWidget(m_bridgeLogView, 1);
    rootLayout->addWidget(logBox, 1);

    m_logRefreshTimer = new QTimer(this);
    m_logRefreshTimer->setInterval(250);

    setCentralWidget(central);
    refreshOptionsSummary();
    refreshPageSummary();
}

void MainWindow::wireSignals()
{
    connect(m_loginButton, &QPushButton::clicked, this, [this]() {
        const QString user = m_usernameEdit->text().trimmed();
        const bool valid = !user.isEmpty() && !m_passwordEdit->text().isEmpty();
        const QString status = valid ? QStringLiteral("登录已提交") : QStringLiteral("登录信息不完整");
        m_statusLabel->setText(QStringLiteral("状态：%1").arg(status));
        appendUiLog(QStringLiteral("点击了登录按钮，用户名：%1").arg(user));
        qInfo().noquote() << QStringLiteral("[界面] 点击登录按钮 用户='%1' 有效=%2")
                                 .arg(user, valid ? QStringLiteral("true") : QStringLiteral("false"));
    });

    connect(m_incrementButton, &QPushButton::clicked, this, [this]() {
        ++m_counter;
        m_counterLabel->setText(QStringLiteral("计数器：%1").arg(m_counter));
        m_statusLabel->setText(QStringLiteral("状态：计数器已增加"));
        appendUiLog(QStringLiteral("计数器增加到 %1").arg(m_counter));
        qInfo().noquote() << QStringLiteral("[界面] 点击增加按钮 计数器=%1").arg(m_counter);
    });

    connect(m_resetButton, &QPushButton::clicked, this, [this]() {
        m_counter = 0;
        m_usernameEdit->clear();
        m_passwordEdit->clear();
        m_counterLabel->setText(QStringLiteral("计数器：0"));
        m_statusLabel->setText(QStringLiteral("状态：已重置"));
        appendUiLog(QStringLiteral("界面状态已重置"));
        qInfo().noquote() << QStringLiteral("[界面] 点击重置按钮");
    });

    connect(m_enableAdvancedCheck, &QCheckBox::toggled, this, [this](bool) { refreshOptionsSummary(); });
    connect(m_modeStandardRadio, &QRadioButton::toggled, this, [this](bool) { refreshOptionsSummary(); });
    connect(m_modeExpertRadio, &QRadioButton::toggled, this, [this](bool) { refreshOptionsSummary(); });
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { refreshOptionsSummary(); });

    connect(m_loadUsersButton, &QPushButton::clicked, this, [this]() {
        m_usersListWidget->clear();
        m_usersProgressBar->setValue(15);
        m_usersStatusLabel->setText(QStringLiteral("用户加载状态：加载中..."));
        m_statusLabel->setText(QStringLiteral("状态：正在异步加载用户"));
        appendUiLog(QStringLiteral("开始异步加载演示用户"));
        qInfo().noquote() << QStringLiteral("[界面] 点击加载演示用户按钮");

        QTimer::singleShot(1200, this, [this]() {
            const QStringList users{
                QStringLiteral("张三 / 管理员"),
                QStringLiteral("李四 / 操作员"),
                QStringLiteral("王五 / 审核员"),
                QStringLiteral("赵六 / 访客"),
            };

            for (const QString& user : users) {
                m_usersListWidget->addItem(user);
            }

            m_usersProgressBar->setValue(100);
            m_usersStatusLabel->setText(QStringLiteral("用户加载状态：%1 条").arg(users.size()));
            m_statusLabel->setText(QStringLiteral("状态：用户加载完成"));
            appendUiLog(QStringLiteral("异步用户加载完成"));
            qInfo().noquote() << QStringLiteral("[界面] 演示用户加载完成 数量=%1").arg(users.size());
        });
    });

    connect(m_revealSecretButton, &QPushButton::clicked, this, [this]() {
        m_statusLabel->setText(QStringLiteral("状态：准备显示隐藏面板"));
        appendUiLog(QStringLiteral("请求显示隐藏面板"));
        qInfo().noquote() << QStringLiteral("[界面] 点击显示隐藏面板按钮");

        QTimer::singleShot(900, this, [this]() {
            m_secretPanel->setVisible(true);
            m_secretStatusLabel->setText(QStringLiteral("隐藏面板已经显示。"));
            m_statusLabel->setText(QStringLiteral("状态：隐藏面板已显示"));
            appendUiLog(QStringLiteral("隐藏面板已经显示"));
            qInfo().noquote() << QStringLiteral("[界面] 隐藏面板已显示");
        });
    });

    connect(m_secretActionButton, &QPushButton::clicked, this, [this]() {
        m_secretStatusLabel->setText(QStringLiteral("隐藏面板操作已确认。"));
        m_statusLabel->setText(QStringLiteral("状态：隐藏操作已确认"));
        appendUiLog(QStringLiteral("点击了隐藏面板确认按钮"));
        qInfo().noquote() << QStringLiteral("[界面] 点击隐藏面板确认按钮");
    });

    connect(m_openConfirmDialogButton, &QPushButton::clicked, this, [this]() {
        appendUiLog(QStringLiteral("打开确认对话框"));
        qInfo().noquote() << QStringLiteral("[界面] 点击打开确认对话框按钮");
        showConfirmDialog();
    });

    connect(m_showFirstPageButton, &QPushButton::clicked, this, [this]() {
        m_pageStackedWidget->setCurrentIndex(0);
        refreshPageSummary();
        appendUiLog(QStringLiteral("切换到第一页"));
    });

    connect(m_showSecondPageButton, &QPushButton::clicked, this, [this]() {
        m_pageStackedWidget->setCurrentIndex(1);
        refreshPageSummary();
        appendUiLog(QStringLiteral("切换到第二页"));
    });

    connect(m_pageStackedWidget, &QStackedWidget::currentChanged, this, [this](int index) {
        refreshPageSummary();
        appendUiLog(QStringLiteral("堆叠页面切换到：%1").arg(index == 0 ? QStringLiteral("第一页") : QStringLiteral("第二页")));
    });

    connect(m_scrollTargetButton, &QPushButton::clicked, this, [this]() {
        m_statusLabel->setText(QStringLiteral("状态：点击了滚动区域目标按钮"));
        appendUiLog(QStringLiteral("点击了滚动区域底部按钮"));
        qInfo().noquote() << QStringLiteral("[界面] 点击滚动区域目标按钮");
    });

    connect(m_demoTreeWidget, &QTreeWidget::itemSelectionChanged, this, [this]() {
        if (QTreeWidgetItem* item = m_demoTreeWidget->currentItem()) {
            appendUiLog(QStringLiteral("树形控件选择了：%1").arg(item->text(0)));
        }
    });

    connect(m_demoTableWidget, &QTableWidget::itemSelectionChanged, this, [this]() {
        if (QTableWidgetItem* item = m_demoTableWidget->currentItem()) {
            appendUiLog(QStringLiteral("表格选择了：%1").arg(item->text()));
        }
    });

    connect(m_demoTabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        appendUiLog(QStringLiteral("切换到标签页：%1").arg(m_demoTabWidget->tabText(index)));
        refreshPageSummary();
    });

    connect(m_logRefreshTimer, &QTimer::timeout, this, &MainWindow::refreshBridgeLog);
    m_logRefreshTimer->start();
    refreshBridgeLog();
}

void MainWindow::appendUiLog(const QString& line)
{
    m_uiLogView->appendPlainText(line);
}

void MainWindow::refreshBridgeLog()
{
    const QJsonArray entries = AppLogSink::instance().recentEntries(60);

    QStringList lines;
    for (const QJsonValue& value : entries) {
        const QJsonObject entry = value.toObject();
        lines.append(QStringLiteral("[%1] %2")
                         .arg(entry.value(QStringLiteral("level")).toString(),
                              entry.value(QStringLiteral("message")).toString()));
    }

    const QString snapshot = lines.join(QStringLiteral("\n"));
    if (snapshot == m_lastBridgeLogSnapshot) {
        return;
    }

    m_lastBridgeLogSnapshot = snapshot;
    m_bridgeLogView->setPlainText(snapshot);
    m_bridgeLogView->verticalScrollBar()->setValue(m_bridgeLogView->verticalScrollBar()->maximum());
}

void MainWindow::refreshOptionsSummary()
{
    const QString mode = m_modeExpertRadio->isChecked() ? QStringLiteral("专家模式") : QStringLiteral("标准模式");
    const QString advanced = m_enableAdvancedCheck->isChecked() ? QStringLiteral("高级模式开启") : QStringLiteral("高级模式关闭");
    m_optionsSummaryLabel->setText(
        QStringLiteral("当前选项：%1 / %2 / %3").arg(mode, m_themeCombo->currentText(), advanced));
}

void MainWindow::refreshPageSummary()
{
    const QString currentPage = m_pageStackedWidget->currentIndex() == 0 ? QStringLiteral("第一页") : QStringLiteral("第二页");
    m_currentPageLabel->setText(QStringLiteral("当前页面：%1").arg(currentPage));
}

void MainWindow::showConfirmDialog()
{
    auto* dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setObjectName(QStringLiteral("confirmDemoDialog"));
    dialog->setWindowTitle(QStringLiteral("确认演示操作"));
    dialog->resize(360, 160);

    auto* layout = new QVBoxLayout(dialog);
    auto* label = new QLabel(QStringLiteral("是否确认执行这次演示操作？"), dialog);
    label->setObjectName(QStringLiteral("confirmDialogLabel"));
    label->setWordWrap(true);
    layout->addWidget(label);

    auto* buttonRow = new QHBoxLayout();
    auto* acceptButton = new QPushButton(QStringLiteral("确认"), dialog);
    acceptButton->setObjectName(QStringLiteral("confirmAcceptButton"));
    auto* cancelButton = new QPushButton(QStringLiteral("取消"), dialog);
    cancelButton->setObjectName(QStringLiteral("confirmCancelButton"));
    buttonRow->addWidget(acceptButton);
    buttonRow->addWidget(cancelButton);
    layout->addLayout(buttonRow);

    connect(acceptButton, &QPushButton::clicked, dialog, [this, dialog]() {
        m_dialogResultLabel->setText(QStringLiteral("对话框结果：已确认"));
        m_statusLabel->setText(QStringLiteral("状态：对话框已确认"));
        appendUiLog(QStringLiteral("确认对话框已确认"));
        qInfo().noquote() << QStringLiteral("[界面] 确认对话框已确认");
        dialog->accept();
    });

    connect(cancelButton, &QPushButton::clicked, dialog, [this, dialog]() {
        m_dialogResultLabel->setText(QStringLiteral("对话框结果：已取消"));
        m_statusLabel->setText(QStringLiteral("状态：对话框已取消"));
        appendUiLog(QStringLiteral("确认对话框已取消"));
        qInfo().noquote() << QStringLiteral("[界面] 确认对话框已取消");
        dialog->reject();
    });

    dialog->show();
}
