
#include "sys_headers.h"
#include "networking.h"
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
#include <QFileDialog>
#include <QtConcurrent>
#include <QFuture>





class WindowGUI : public QMainWindow  {

    Q_OBJECT
    

    public:
        WindowGUI(QWidget *parent = nullptr);

        void setMainWindowContent();
        void setMainWindowContent(Library* library, string dispText = "Local Library");
        void setLocalLibrary(Library* library);
        

    private:
        QWidget* mainBox;
        Library* localLibrary;
        QFutureWatcher<void> watcher;

        void connClientCallback();
        void ChangeLblText(string text);
        void showDirSelect();
        void changeOpMode();
        void startSyncFunc();
};


class AlbumDropdown : public QWidget {

    Q_OBJECT

    public:
        AlbumDropdown(unique_ptr<Album>& album, QWidget *parent = nullptr);

    signals:
        void leftClicked();
        void rightClicked();

    public slots:
        void onLeftClick();
        void onRightClick();


    protected:
        void mousePressEvent(QMouseEvent* event) override {
            if(event->button() == Qt::LeftButton){
                if(layout() && layout()->count() > 0){
                    QWidget* titleBar = layout()->itemAt(0)->widget();
                    QWidget* clickedWidget = childAt(event->pos());
                    if((titleBar == clickedWidget || titleBar->isAncestorOf(clickedWidget))){
                        emit leftClicked();
                    }
                }
            }else if(event->button() == Qt::RightButton){
                emit rightClicked();
            }
            QWidget::mousePressEvent(event);
        }

    private:
        bool open;
        const Album* libAlbum;
        QVBoxLayout* albumLayout;
        //void ChangeLblText(string text);
};