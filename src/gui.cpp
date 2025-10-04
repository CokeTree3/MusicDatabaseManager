#include "gui.h"
#include "qboxlayout.h"
#include "qlabel.h"
#include "qlayout.h"
#include "qlayoutitem.h"
#include "qlist.h"
#include "qnamespace.h"
#include "qobject.h"
#include "qpixmap.h"
#include "qscrollarea.h"
#include "qsizepolicy.h"
#include "qwidget.h"





WindowGUI::WindowGUI(QWidget* parent) :QMainWindow(parent) {
    resize(500, 500);
    setWindowTitle("Hello Qt :)");

    QWidget* mainWindowBox = new QWidget(this);
    setCentralWidget(mainWindowBox);

    auto* vbox = new QVBoxLayout(mainWindowBox);

    auto *quit = new QAction("&Quit", this);
    QMenu *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(quit);

    auto *changeText = new QAction("&Update Text", this);
    QMenu *editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction(changeText);

    QPushButton *btn = new QPushButton("Click Me", this);
    //mainTextBox = new QTextEdit("", mainWindowBox);
    //mainTextBox->setReadOnly(true);
    //mainTextBox->setFrameStyle(QFrame::NoFrame);

    mainBox = new QWidget();
    mainBox->setLayout(new QVBoxLayout());

    mainScrollArea = new QScrollArea();
    mainScrollArea->setWidget(mainBox);
    mainScrollArea->setWidgetResizable(true);
    mainScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    vbox->addWidget(mainScrollArea, 1);
    vbox->addWidget(btn, 1, Qt::AlignRight);
    mainWindowBox->setLayout(vbox);

    //closeButton->setToolTip("Close the window");
    //connect(closeButton, &QPushButton::clicked, qApp, &QApplication::quit);
    connect(changeText, &QAction::triggered, this, [this]{WindowGUI::ChangeLblText("Local Library\n\n1. ArtistName1\n\tAlbumName1 (2 Tracks)\n\t\t1. SongName1\n\t\t2. SongName2"); });
    connect(quit, &QAction::triggered, qApp, QApplication::quit);

}

void WindowGUI::ChangeLblText(string text) {
    cout << "edit to " << text << endl;
}

void WindowGUI::PrintText(){
    cout << "pressed" << endl;
}

void WindowGUI::setmainWindowContent(Library* library){

    QVBoxLayout* mainBoxLayout = (QVBoxLayout*)mainBox->layout();

    QLabel* libLbl = new QLabel("<div style='font-size:13pt;'><b>Local Library</b></div>");
    libLbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    libLbl->setIndent(100);
    mainBoxLayout->addWidget(libLbl, 0);


    for(size_t i = 0; i < library->artistList.size(); i++){
        QLabel* artistLabel = new QLabel("<div style='font-size:13pt;'>" +  QString::number(i+1) + ". " + QString::fromStdString(library->artistList[i]->name) + "</div>");
        artistLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        mainBoxLayout->addWidget(artistLabel, 0);
        for(size_t j = 0; j < library->artistList[i]->albumCount; j++){
            
            AlbumDropdown* albumLine = new AlbumDropdown(library->artistList[i]->albumList[j]);
            mainBoxLayout->addWidget(albumLine);
            
            connect(albumLine, &AlbumDropdown::clicked, albumLine, &AlbumDropdown::onClick);
        }
    }
}








AlbumDropdown::AlbumDropdown(unique_ptr<Album>& album, QWidget* parent) : QWidget(parent){
    open = false;
    this->libAlbum = album.get();
    albumLayout = new QVBoxLayout();

    QHBoxLayout* albumTitleLayout = new QHBoxLayout();
    
    QLabel* pixBox = new QLabel();
    QPixmap* pix = new QPixmap(QString::fromStdString(album->coverPath));
    pixBox->setPixmap(pix->scaled(60, 60, Qt::KeepAspectRatio, Qt::FastTransformation));
    pixBox->setFixedSize(60, 60);
    pixBox->setAttribute(Qt::WA_TransparentForMouseEvents);

    QLabel* albumLabel = new QLabel("<div style='font-size:13pt;'>" + QString::fromStdString(album->name) + "</div>");
    albumLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    albumLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    albumTitleLayout->addWidget(pixBox);
    albumTitleLayout->addWidget(albumLabel);
    albumTitleLayout->setContentsMargins(20, 5, 0, 5);

    albumLayout->addLayout(albumTitleLayout);

    setMouseTracking(true);
    setLayout(albumLayout);
}

void AlbumDropdown::onClick(){
    if(!open){
        for(size_t i = 0; i < libAlbum->trackCount; i++){
            QLabel* trackLabel = new QLabel("<div style='font-size:11pt;'>"+ QString::number(i+1) + ". " + QString::fromStdString(libAlbum->trackList[i]->name) + "</div>");
            trackLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
            trackLabel->setIndent(50);
            albumLayout->addWidget(trackLabel);
        }
    }else{
        for(size_t i = albumLayout->count()-1; i > 0; i--){
            QLayoutItem* item = albumLayout->takeAt(i);
            if (item) {
                QWidget* widget = item->widget();
                if (widget) {
                    delete widget;
                }
                delete item;
            }
        }
    }
    open = !open;
    cout << endl;
}