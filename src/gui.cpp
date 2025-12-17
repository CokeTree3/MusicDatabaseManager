#include "gui.h"

int deleteQWidgetFromLayout(QLayout* layout, int indexInLayout){
    QLayoutItem* item = layout->takeAt(indexInLayout);
    if (item) {
        QWidget* widget = item->widget();
        if (widget) {
            widget->setParent(nullptr);
            delete widget;
        }else return 1;
        delete item;
    }else return 1;

    return 0;
}


WindowGUI::WindowGUI(QWidget* parent) :QMainWindow(parent) {
    resize(500, 500);
    setWindowTitle("Music Database Manager");

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
    auto *rescanBtn = new QAction("Rescan Library", this);
    auto *tempSel = new QAction("&exec", this);
    QMenu *editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction(changeText);
    editMenu->addAction(tempSel);
    editMenu->addAction(rescanBtn);

    auto *changeRemoveStatusSelect = new QAction("&Allow removing local tracks", this);
    changeRemoveStatusSelect->setCheckable(true);

    QMenu *settingsMenu = menuBar()->addMenu("&Settings");
    settingsMenu->addAction(changeRemoveStatusSelect);


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
    connect(changeRemoveStatusSelect, &QAction::triggered, this, &WindowGUI::changeRemoveStatus);

    //connect(tempSel, &QAction::triggered, this, &WindowGUI::showSyncSelection);

    connect(syncBtn, &QPushButton::clicked, this, &WindowGUI::startSyncFunc);
    connect(&watcher, &QFutureWatcher<int>::finished, this, [this]{WindowGUI::connCallback(watcher.result());});
}

#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)
WindowGUI::~WindowGUI(){
    if(localLibrary != nullptr){
        if(localLibrary->serverActive){
            stopServer();
        }
    }
}
#else
WindowGUI::~WindowGUI(){
    if(localLibrary != nullptr){
        // more graceful networking stop
    }
}
#endif

void WindowGUI::ChangeLblText(string text) {
    cout << "edit to " << text << endl;
}

void WindowGUI::showErrorPopup(int type, string text){
    switch(type){
    case 0:
        QMessageBox::critical(this, tr("Error!"), QString::fromStdString(text));
        break;
    case 1:
        QMessageBox::warning( this, tr("Warning"), QString::fromStdString(text));
        break;
    case 2:
        
    default:
        QMessageBox::information( this, tr("Information"), QString::fromStdString(text));
        break;
    }
}

void WindowGUI::changeOpMode(){
    if(!libSet){
        (qobject_cast<QAction*>(sender()))->setChecked(false);
        showErrorPopup(2, "Please set the local library path first!");
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

void WindowGUI::changeRemoveStatus(){
    if(!libSet){
        (qobject_cast<QAction*>(sender()))->setChecked(false);
        showErrorPopup(2, "Please set the local library path first!");
        return;
    }

    cout << "Not yet implemented!" << endl;                             // TODO implement the switch, variable in library not gui

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

#if defined (PLATFORM_ANDROID)
        showErrorPopup(1, "Server operations not supported on current platform!");
        return;
#endif

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
        showErrorPopup(1, "Please only select one library directory!");
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
            showErrorPopup(0, "Failed connecting to the server at the provided address!");

        }else{
            cout << "calling sync" << endl;
            Library diffLib;
            if(localLibrary->generateDiff(diffLib) == 1){
                showErrorPopup(0, "Failed synchronizing data with the server!");
                goto syncDone;
            }

            if(diffLib.artistList.empty()){
                showErrorPopup(2, "The local library is up to date!");
                goto syncDone;
            }

            showSyncSelection(diffLib);

            if(diffLib.permsObtained){                                  // Skips if the selection window was closed manually, callback wasn't called
                if(localLibrary->implementDiff(diffLib) == 1){
                    showErrorPopup(0, "Failed implementing the server sync data!");
                }
                cout << "sync finished\n";
            }

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
            showErrorPopup(0, "Error operating the server!");
        }
        QPushButton* btn = qobject_cast<QPushButton*>(this->centralWidget()->layout()->itemAt(this->centralWidget()->layout()->count() - 2)->widget());

        if(!btn){
            showErrorPopup(1, "General Error");
            goto syncDone;
        }
        btn->setText("Start Server");

        disconnect(btn, &QPushButton::clicked, this, nullptr);
        connect(btn, &QPushButton::clicked, this, &WindowGUI::startSyncFunc);
    }

    syncDone:

    QVBoxLayout* layout = (QVBoxLayout*)centralWidget()->layout();
    if(deleteQWidgetFromLayout(layout, layout->count() - 1)) showErrorPopup(1, "General Error");
}

void WindowGUI::showSyncSelection(Library &diffLib){

    cout << "DiffLib has: " << diffLib.artistList.size() << " artists" << endl;

    QDialog* syncSelectWindow = new QDialog(this);
    syncSelectWindow->setWindowFlags(Qt::Tool);
    syncSelectWindow->setModal(true);
    syncSelectWindow->setWindowTitle("Select What to Synchronize");

    vector<QCheckBox*> checkBoxList;

    auto* outerBoxLayout = new QVBoxLayout(syncSelectWindow);

    QTreeWidget* selectionTree = new QTreeWidget(syncSelectWindow);
    
    selectionTree->setHeaderHidden(true);
    selectionTree->setColumnCount(2);
    selectionTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    selectionTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QPushButton* selectBtn = new QPushButton("Select");

    connect(selectBtn, &QPushButton::clicked, syncSelectWindow, [this, &syncSelectWindow, &diffLib, &selectionTree]{
        WindowGUI::syncSelectionCallback(diffLib, selectionTree);
        syncSelectWindow->accept();
        });

    outerBoxLayout->addWidget(selectionTree, 1);
    outerBoxLayout->addWidget(selectBtn, 0);
    syncSelectWindow->setLayout(outerBoxLayout);

    QHeaderView *h = selectionTree->header();
    h->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    h->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    h->setStretchLastSection(false);

    for(size_t i = 0; i < diffLib.artistList.size(); i++){
        QTreeWidgetItem* artistLine = new QTreeWidgetItem(selectionTree);

        artistLine->setFlags(artistLine->flags() | Qt::ItemIsAutoTristate);
        setUpSelectionTreeItemLine(artistLine, QString::fromStdString(diffLib.artistList[i]->name), diffLib.artistList[i]->toBeRemoved);
        bool albumRemoved = false;

        for(size_t j = 0; j < diffLib.artistList[i]->albumCount; j++){
            QTreeWidgetItem* albumLine = new QTreeWidgetItem(artistLine);

            albumLine->setFlags(albumLine->flags() | Qt::ItemIsAutoTristate);
            setUpSelectionTreeItemLine(albumLine, QString::fromStdString(diffLib.artistList[i]->albumList[j]->name), diffLib.artistList[i]->albumList[j]->toBeRemoved);
            bool trackRemoved = false;

            if(diffLib.artistList[i]->albumList[j]->toBeRemoved){
                albumRemoved = true;
            }
            for(size_t k = 0; k < diffLib.artistList[i]->albumList[j]->trackCount; k++){
                QTreeWidgetItem* trackLine = new QTreeWidgetItem(albumLine);
                
                setUpSelectionTreeItemLine(trackLine, QString::fromStdString(diffLib.artistList[i]->albumList[j]->trackList[k]->name), diffLib.artistList[i]->albumList[j]->trackList[k]->toBeRemoved);
                if(diffLib.artistList[i]->albumList[j]->trackList[k]->toBeRemoved){
                    trackRemoved = true;
                }
            }

            if(trackRemoved){
                albumLine->setText(1,"Track being Removed!");
                QFont font;
                font.setBold(true);
                albumLine->setFont(1, font);
                albumRemoved = true;
            }
        }

        if(albumRemoved){
            artistLine->setText(1,"Album being Removed!");
            QFont font;
            font.setBold(true);
            artistLine->setFont(1, font);
        }
    }
    
    selectionTree->expandAll();
    selectionTree->resizeColumnToContents(0);
    selectionTree->resizeColumnToContents(1);
    selectionTree->collapseAll();

    syncSelectWindow->exec();
    
}

void WindowGUI::setUpSelectionTreeItemLine(QTreeWidgetItem* line, QString name, bool toRemove){
    line->setText(0, name);
    line->setCheckState(0, Qt::Checked);
    line->setFlags(line->flags() | Qt::ItemIsUserCheckable);
    if(toRemove){
        line->setText(1,"To Remove!");
        QFont font;
        font.setBold(true);
        line->setFont(1, font);
    }
}

void WindowGUI::syncSelectionCallback(Library& diffLib, QTreeWidget* selectionList){
    QTreeWidgetItemIterator it(selectionList);

    diffLib.permsObtained = true;

    for(size_t i = 0; i < diffLib.artistList.size(); i++){
        if(*it && (*it)->checkState(0) == Qt::Unchecked){
            cout << diffLib.artistList[i]->name << " Artist is unchecked and removed from diff" << endl;
            diffLib.removeFromLibrary(diffLib.artistList[i]->name, i);
            continue;
        }
        ++it;
        for(size_t j = 0; j < diffLib.artistList[i]->albumCount; j++){
            if(*it && (*it)->checkState(0) == Qt::Unchecked){
                cout << diffLib.artistList[i]->albumList[j]->name << " Album is unchecked and removed from diff" << endl;
                diffLib.artistList[i]->removeAlbum(diffLib.artistList[i]->albumList[j]->name, j);
                continue;
            }
            ++it;
            for(size_t k = 0; k < diffLib.artistList[i]->albumList[j]->trackCount; k++){
                if(*it && (*it)->checkState(0) == Qt::Unchecked){
                    cout << diffLib.artistList[i]->albumList[j]->trackList[k]->name << " Track is unchecked and removed from diff" << endl;
                    diffLib.artistList[i]->albumList[j]->removeTrack(diffLib.artistList[i]->albumList[j]->trackList[k]->name, k);
                }
                ++it;
            }
        }
    }
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

    if(library->artistList.empty()){
        mainBoxLayout->setAlignment(Qt::AlignTop);
        return;
    }

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
    QPixmap* pix = new QPixmap();
    if(album->coverPath == "" && album->coverBuf.empty()){
        pix->load(QDir::currentPath() + "/assets/missingCov.jpg");
    }else if (album->coverPath == ""){
        pix->loadFromData(album->coverBuf.data(), album->coverBuf.size());
    }else{
        pix->load(QString::fromStdString(album->coverPath));
    }

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
