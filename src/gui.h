
#include "qboxlayout.h"
#include "qobject.h"
#include "qpushbutton.h"
#include "qscrollarea.h"
#include "qwidget.h"
#include "sys_headers.h"
#include "library.h"
#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QTextEdit>
#include <QScrollArea>
#include <QPixmap>
#include <QMouseEvent>
#include <memory>



class WindowGUI : public QMainWindow  {

    Q_OBJECT

    public:
        WindowGUI(QWidget *parent = nullptr);

        void setmainWindowContent(Library* library);
        
        

    private:
        QWidget* mainBox;
        QScrollArea* mainScrollArea;

        void PrintText();
        void ChangeLblText(string text);
};


class AlbumDropdown : public QWidget {

    Q_OBJECT

    public:
        AlbumDropdown(unique_ptr<Album>& album, QWidget *parent = nullptr);

    signals:
        void clicked();

    public slots:
        void onClick();


    protected:
        void mousePressEvent(QMouseEvent* event) override {
            if(event->button() == Qt::LeftButton){
                emit clicked();
            }
            QWidget::mousePressEvent(event);
        }

    private:
        bool open;
        const Album* libAlbum;
        QVBoxLayout* albumLayout;
        //void ChangeLblText(string text);
};