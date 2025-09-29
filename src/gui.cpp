#include "gui.h"
#include "qaction.h"
#include "qmainwindow.h"


WindowGUI::WindowGUI(QWidget* parent) :QMainWindow(parent) {
    resize(500, 500);
    setWindowTitle("Hello Qt :)");

    auto *quit = new QAction("&Quit", this);
    QMenu *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(quit);

    auto *changeText = new QAction("&Update Text", this);
    QMenu *editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction(changeText);
    
    

    //QPushButton *closeButton = new QPushButton("Close Window", this);
    //QPushButton *btn = new QPushButton("Click Me", this);
    lbl = new QLabel("0", this);

    //auto *grid = new QGridLayout(this);
    //grid->addWidget(btn, 0, 0);
    ///grid->addWidget(lbl, 0, 1);
    //grid->addWidget(closeButton, 2, 1);

    //setLayout(grid);

    lbl->setGeometry(200, 200, 100, 40);
    //closeButton->setToolTip("Close the window");
    //connect(closeButton, &QPushButton::clicked, qApp, &QApplication::quit);
    connect(changeText, &QAction::triggered, this, [this]{WindowGUI::ChangeLblText("qwew"); });
    connect(quit, &QAction::triggered, qApp, QApplication::quit);
}

void WindowGUI::ChangeLblText(string text) {
    

    lbl->setText(QString::fromStdString(text));
}