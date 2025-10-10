#include "gui.h"
#include "library.h"


void deleteQWidgetFromLayout(QLayout* layout, int indexInLayout){
    QLayoutItem* item = layout->takeAt(indexInLayout);
    if (item) {
        QWidget* widget = item->widget();
        if (widget) {
            delete widget;
        }
        delete item;
    }
}


WindowGUI::WindowGUI(QWidget* parent) :QMainWindow(parent) {
    resize(500, 500);
    setWindowTitle("Hello Qt :)");

    QWidget* mainWindowBox = new QWidget(this);
    setCentralWidget(mainWindowBox);

    auto* vbox = new QVBoxLayout(mainWindowBox);

    auto *quit = new QAction("&Quit", this);
    auto *libraryPath = new QAction("&Path to Library", this);
    QMenu *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(quit);
    fileMenu->addAction(libraryPath);

    auto *changeText = new QAction("&Update Text", this);
    QMenu *editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction(changeText);

    QPushButton *btn = new QPushButton("Click Me", this);
    QLabel* noLibraryNotification = new QLabel("No Library found, please set a path to a valid library in File menu!");
    //mainTextBox = new QTextEdit("", mainWindowBox);
    //mainTextBox->setReadOnly(true);
    //mainTextBox->setFrameStyle(QFrame::NoFrame);

    QScrollArea* mainScrollArea = new QScrollArea(mainWindowBox);
    mainBox = new QWidget(mainScrollArea);
    mainBox->setLayout(new QVBoxLayout());

    
    mainScrollArea->setWidget(mainBox);
    mainScrollArea->setWidgetResizable(true);
    mainScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    vbox->addWidget(mainScrollArea, 1);
    vbox->addWidget(noLibraryNotification, 0);
    vbox->addWidget(btn, 1, Qt::AlignRight);
    mainWindowBox->setLayout(vbox);

    //closeButton->setToolTip("Close the window");
    //connect(closeButton, &QPushButton::clicked, qApp, &QApplication::quit);
    connect(changeText, &QAction::triggered, this, [this]{WindowGUI::ChangeLblText("Local Library\n\n1. ArtistName1\n\tAlbumName1 (2 Tracks)\n\t\t1. SongName1\n\t\t2. SongName2"); });
    connect(quit, &QAction::triggered, qApp, QApplication::quit);
    connect(libraryPath, &QAction::triggered, this, &WindowGUI::showDirSelect);
}

void WindowGUI::ChangeLblText(string text) {
    cout << "edit to " << text << endl;
}

void WindowGUI::showDirSelect(){
    QFileDialog selectDialog(this);
    selectDialog.setFileMode(QFileDialog::Directory);
    selectDialog.setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    while (selectDialog.exec()){
        fileNames = selectDialog.selectedFiles();
        if(fileNames.count() == 1){
            break;
        }
        QMessageBox::warning( this, tr("Error"), tr("Please only select one library direcotry!"));
    }

    if(fileNames.count() == 1){
        localLibrary->resetLibrary();
        localLibrary->buildLibrary( fileNames[0].toStdString());

        if(mainBox->layout()->count() > 1){
            for(int i = mainBox->layout()->count()-1; i >= 0; i--){
                deleteQWidgetFromLayout(mainBox->layout(), i);
            }
        }
        
        this->setMainWindowContent();
    }

}

void WindowGUI::setLocalLibrary(Library* library){
    this->localLibrary = library;
}

void WindowGUI::PrintText(){
    cout << "pressed" << endl;
}

void WindowGUI::setMainWindowContent(){
    this->setMainWindowContent(localLibrary);
}

void WindowGUI::setMainWindowContent(Library* library){
    if(this->centralWidget()->layout()->count() == 3){
        deleteQWidgetFromLayout(this->centralWidget()->layout(), 1);
    }
    
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
            
            connect(albumLine, &AlbumDropdown::leftClicked, albumLine, &AlbumDropdown::onLeftClick);
            connect(albumLine, &AlbumDropdown::rightClicked, albumLine, &AlbumDropdown::onRightClick);
            
        }
    }
}



AlbumDropdown::AlbumDropdown(unique_ptr<Album>& album, QWidget* parent) : QWidget(parent){
    open = false;
    this->libAlbum = album.get();
    albumLayout = new QVBoxLayout();

    QHBoxLayout* albumTitleLayout = new QHBoxLayout();
    QWidget* albumTitleBar = new QWidget();

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

    albumTitleBar->setLayout(albumTitleLayout);
    albumLayout->addWidget(albumTitleBar);

    setLayout(albumLayout);
}

void AlbumDropdown::onLeftClick(){
    if(!open){
        for(size_t i = 0; i < libAlbum->trackCount; i++){
            QLabel* trackLabel = new QLabel("<div style='font-size:11pt;'>"+ QString::number(i+1) + ". " + QString::fromStdString(libAlbum->trackList[i]->name) + "</div>");
            trackLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
            trackLabel->setIndent(50);
            albumLayout->addWidget(trackLabel);
            
        }
    }else{
        for(size_t i = albumLayout->count()-1; i > 0; i--){
            deleteQWidgetFromLayout(albumLayout, i);
        }
    }
    open = !open;
}

void AlbumDropdown::onRightClick(){
    cout << "sdasd" << endl;
    
}