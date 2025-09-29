
#include "qmainwindow.h"
#include "qwidget.h"
#include "sys_headers.h"
#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>
#include <QLabel>
#include <QGridLayout>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>


class WindowGUI : public QMainWindow  {

    Q_OBJECT

    public:
        WindowGUI(QWidget *parent = nullptr);

    private slots:
        void ChangeLblText(string text);
        

    private:
        QLabel *lbl;

};
