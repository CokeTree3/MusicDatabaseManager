#include "gui.h"
#include "qobject.h"

void deleteQWidgetFromLayout(QLayout* layout, int indexInLayout){
    QLayoutItem* item = layout->takeAt(indexInLayout);
    if (item) {
        QWidget* widget = item->widget();
        if (widget) {
            widget->setParent(nullptr);
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
    auto *opModeSelect = new QAction("&Enable Server mode", this);
    opModeSelect->setCheckable(true);
    QMenu *fileMenu = menuBar()->addMenu("&File");

    fileMenu->addAction(libraryPath);
    fileMenu->addAction(opModeSelect);
    fileMenu->addAction(quit);

    auto *changeText = new QAction("&Update Text", this);
    QMenu *editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction(changeText);

    QPushButton *syncBtn = new QPushButton("Start Sync", this);
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
    vbox->addWidget(syncBtn, 1, Qt::AlignRight);
    mainWindowBox->setLayout(vbox);

    //closeButton->setToolTip("Close the window");
    //connect(closeButton, &QPushButton::clicked, qApp, &QApplication::quit);
    connect(changeText, &QAction::triggered, this, [this]{WindowGUI::ChangeLblText("Local Library\n\n1. ArtistName1\n\tAlbumName1 (2 Tracks)\n\t\t1. SongName1\n\t\t2. SongName2"); });
    connect(quit, &QAction::triggered, qApp, QApplication::quit);
    connect(libraryPath, &QAction::triggered, this, &WindowGUI::showDirSelect);
    connect(opModeSelect, &QAction::triggered, this, &WindowGUI::changeOpMode);

    connect(syncBtn, &QPushButton::clicked, this, &WindowGUI::startSyncFunc);
    connect(&watcher, &QFutureWatcher<int>::finished, this, [this]{WindowGUI::connCallback(watcher.result());});
}

WindowGUI::~WindowGUI(){
    if(localLibrary != nullptr){
        if(localLibrary->serverActive){                                 // server needs to be stopped
            stopServer();
        }
    }
}

void WindowGUI::ChangeLblText(string text) {
    cout << "edit to " << text << endl;
}

void WindowGUI::changeOpMode(){
    if(!libSet){
        (qobject_cast<QAction*>(sender()))->setChecked(false);
        QMessageBox::warning( this, tr("Error"), tr("Please set the local library path first!"));
        return;
    }
    if(serverMode){
        //change to client mode
        cout << "to client mode\n";
        QPushButton* btn = qobject_cast<QPushButton*>(this->centralWidget()->layout()->itemAt(this->centralWidget()->layout()->count() - 1)->widget());
        btn->setText("Start Sync");
    }else{
        cout << "to server mode\n";
        QPushButton* btn = qobject_cast<QPushButton*>(this->centralWidget()->layout()->itemAt(this->centralWidget()->layout()->count() - 1)->widget());
        btn->setText("Start Server");

    }
    serverMode = !serverMode;
}



void WindowGUI::startSyncFunc(){
    if(!serverMode){
        

        bool ok{};
        QString text = QInputDialog::getText(this, tr("Server address"),
                                             tr("IP of the server:"), QLineEdit::Normal,
                                             tr("127.0.0.1"), &ok);
        if (ok && !text.isEmpty()){
            QVBoxLayout* layout = (QVBoxLayout*)centralWidget()->layout();
            QLabel* syncLbl = new QLabel("Synchronizing with the server...");
            layout->addWidget(syncLbl, 0, Qt::AlignRight);

            remoteAddr = text.toStdString();

#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)
            QFuture<int> future = QtConcurrent::run([this, &text]() {
                return initConn(&localLibrary->remoteLibJson,  remoteAddr);
            });
            watcher.setFuture(future);
#elif defined (PLATFORM_ANDROID)                                                                    // Add multithreading support when QNetwork is used (QFuture not supported)
            int ret = initConn(&localLibrary->remoteLibJson,  remoteAddr);
            connCallback(ret);
#endif
        }
    }
    else{

#if defined(PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)
        QPushButton* btn = qobject_cast<QPushButton*>(this->centralWidget()->layout()->itemAt(this->centralWidget()->layout()->count() - 1)->widget());
        btn->setText("Stop Server");

        disconnect(btn, &QPushButton::clicked, this, &WindowGUI::startSyncFunc);
        connect(btn, &QPushButton::clicked, this, &stopServer);

        QVBoxLayout* layout = (QVBoxLayout*)centralWidget()->layout();
        QLabel* syncLbl = new QLabel("server operational...");
        layout->addWidget(syncLbl, 0, Qt::AlignRight);

        localLibrary->serverActive = true;

        QFuture<int> future = QtConcurrent::run([this]() {
            return initConn(&localLibrary->libJson);
        });
        watcher.setFuture(future);
        //int ret = initConn(&localLibrary->remoteLibJson,  remoteAddr);
        //connCallback(ret);
#elif defined (PLATFORM_ANDROID)    
        cout << "Server operations not supported on current platform!\n";
        QMessageBox::information(this, "Warning", "Server operations not supported on current platform!");

#endif
    }
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
        QMessageBox::warning( this, tr("Error"), tr("Please only select one library directory!"));
    }

    if(fileNames.count() == 1){
        localLibrary->resetLibrary();
        localLibrary->buildLibrary( fileNames[0].toStdString());

        libSet = true;

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


void WindowGUI::connCallback(int connState){
    if(!serverMode){
        if(connState > 0){
            QMessageBox::information(this, "Warning", "Failed connecting to the server at the provided address!");
            
        }else{
            
            // popup for sync selection

            cout << "calling sync\n";
            localLibrary->syncWithServer();
            cout << "sync finished\n";
            
            localLibrary->resetLibrary();       
            localLibrary->jsonRead();                                           // Not needed if new(synced) data is already added to lib(not yet implemented)

            if(mainBox->layout()->count() > 1){                                 // reloads whole window instead of adding the new entries(even if no change to library)
                for(int i = mainBox->layout()->count()-1; i >= 0; i--){
                    deleteQWidgetFromLayout(mainBox->layout(), i);
                }
            }
            this->setMainWindowContent();
        }   
        
    }else{
        localLibrary->serverActive = false;
        if(connState > 0){
            QMessageBox::information(this, "Warning", "Server Error!");
        }
        QPushButton* btn = qobject_cast<QPushButton*>(this->centralWidget()->layout()->itemAt(this->centralWidget()->layout()->count() - 2)->widget());
        btn->setText("Start Server");

        disconnect(btn, &QPushButton::clicked, this, nullptr);
        connect(btn, &QPushButton::clicked, this, &WindowGUI::startSyncFunc);
        
    }

    QVBoxLayout* layout = (QVBoxLayout*)centralWidget()->layout();
    deleteQWidgetFromLayout(layout, layout->count() - 1);
    // update GUI
}

void WindowGUI::setMainWindowContent(){
    this->setMainWindowContent(localLibrary);
}

void WindowGUI::setMainWindowContent(Library* library, string dispText){
    if(this->centralWidget()->layout()->count() == 3){
        QWidget* noLibNotif = this->centralWidget()->layout()->itemAt(1)->widget();
        if(noLibNotif && qobject_cast<QLabel*>(noLibNotif)){
            deleteQWidgetFromLayout(this->centralWidget()->layout(), 1);
        }
    }

    QVBoxLayout* mainBoxLayout = (QVBoxLayout*)mainBox->layout();
    string titleText = string("<div style='font-size:13pt;'><b>") + dispText + string("</b></div>");
    QLabel* libLbl = new QLabel(QString::fromStdString(titleText));
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
