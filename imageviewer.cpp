#include <QtWidgets>
#include <QRubberBand>
#include "imageviewer.h"

ImageViewer::ImageViewer()
   : imageLabel(new QLabel)
   , scrollArea(new QScrollArea)
   , scaleFactor(1)
{
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(true);

    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(imageLabel);
    scrollArea->setVisible(false);
    setCentralWidget(scrollArea);

    createActions();

    resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
}


bool ImageViewer::loadFile(const QString &filename)
{
    fileName = filename;
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage newImage = reader.read();
    if (newImage.isNull()) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot load %1: %2")
                                 .arg(QDir::toNativeSeparators(fileName), reader.errorString()));
        return false;
    }

    redo_stack.clear();
    setImage(newImage);

    setWindowFilePath(fileName);

    const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
        .arg(QDir::toNativeSeparators(fileName)).arg(image.width()).arg(image.height()).arg(image.depth());
    statusBar()->showMessage(message);
    return true;
}

void ImageViewer::setImage(const QImage &newImage)
{
    image = newImage;
    imageLabel->setPixmap(QPixmap::fromImage(image));
    scaleFactor = 1.0;

    scrollArea->setVisible(true);
    updateActions();
    imageLabel->adjustSize();
    setEle();
    undoAct->setEnabled(false);
    if(undo_stack.size() > 1){
        undoAct->setEnabled(true);
    }
}

static void initializeImageFileDialog(QFileDialog &dialog, QFileDialog::AcceptMode acceptMode)
{
    static bool firstDialog = true;

    if (firstDialog) {
        firstDialog = false;
        const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
    }

    QStringList mimeTypeFilters;
    const QByteArrayList supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen
        ? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
    foreach (const QByteArray &mimeTypeName, supportedMimeTypes)
        mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/jpeg");
    if (acceptMode == QFileDialog::AcceptSave)
        dialog.setDefaultSuffix("jpg");
}

void ImageViewer::open()
{
    QFileDialog dialog(this, tr("Open File"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);

    while (dialog.exec() == QDialog::Accepted && !loadFile(dialog.selectedFiles().first())) {}
}

void ImageViewer::reset()
{
    loadFile(fileName);
}

void ImageViewer::save()
{
    imageLabel->pixmap()->save(fileName);
}

void ImageViewer::saveAs()
{
    QStringList mimeTypeFilters;
    foreach(const QByteArray & mimeTypeName , QImageReader::supportedMimeTypes()) // loop through all the supported MimeTypes in the image reader by the bytearray then push them in our mimes
        mimeTypeFilters.append(mimeTypeName); // ?
    mimeTypeFilters.sort();
    QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation); // get the standard location and we will provide to it the standard location type :3
    QFileDialog dialog(this, tr("Save File"), picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last()); // container , what is the windows name :3
    dialog.setAcceptMode(QFileDialog::AcceptSave); // this dialog only for accepting the opening of files :3
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/jpeg");

    dialog.exec();
    imageLabel->pixmap()->save(dialog.selectedFiles().first());
}

void ImageViewer::setEle()
{
    StackElement ele;
    ele.image = image;
    ele.factor = scaleFactor;
    undo_stack.push(ele);
    undoAct->setEnabled(true);
    redoAct->setEnabled(false);
}

void ImageViewer::zoomSelect()
{
    zoomSelection(QRect(start, end));
    redo_stack.clear();
    setEle();
}

void ImageViewer::zoomIn()
{
    scaleImage(1.25);
    redo_stack.clear();
    setEle();
}

void ImageViewer::zoomOut()
{
    scaleImage(0.8);
    redo_stack.clear();
    setEle();
}

void ImageViewer::zoomByFactor()
{
    double factor = QInputDialog::getDouble(this, "", "Enter Factor");
    scaleImage(factor);
    redo_stack.clear();
    setEle();
}

void ImageViewer::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *openAct = fileMenu->addAction(tr("&Open..."), this, &ImageViewer::open);
    openAct->setShortcut(QKeySequence::Open);

    saveAct = fileMenu->addAction(tr("Save"), this, &ImageViewer::save);
    saveAct->setShortcut(tr("Ctrl+S"));
    saveAct->setEnabled(false);

    saveAsAct = fileMenu->addAction(tr("&Save As..."), this, &ImageViewer::saveAs);
    saveAsAct->setShortcut(tr("Ctrl+E"));
    saveAsAct->setEnabled(false);

    fileMenu->addSeparator();

    QAction *exitAct = fileMenu->addAction(tr("E&xit"), this, &QWidget::close);
    exitAct->setShortcut(tr("Ctrl+Q"));

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));

    rotateAct = editMenu->addAction(tr("Rotate"), this, &ImageViewer::rotate);
    rotateAct->setEnabled(false);

    undoAct = editMenu->addAction(tr("Undo"), this, &ImageViewer::undo);
    undoAct->setEnabled(false);

    redoAct = editMenu->addAction(tr("Redo"), this, &ImageViewer::redo);
    redoAct->setEnabled(false);

    resetAct = editMenu->addAction(tr("Reset"), this, &ImageViewer::reset);
    resetAct->setEnabled(false);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    zoomInAct = viewMenu->addAction(tr("Zoom &In (25%)"), this, &ImageViewer::zoomIn);
    zoomInAct->setShortcut(QKeySequence::ZoomIn);
    zoomInAct->setEnabled(false);

    zoomOutAct = viewMenu->addAction(tr("Zoom &Out (25%)"), this, &ImageViewer::zoomOut);
    zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    zoomOutAct->setEnabled(false);

    zoomByFactorAct = viewMenu->addAction(tr("Zoom By Factor"), this, &ImageViewer::zoomByFactor);
    zoomByFactorAct->setEnabled(false);

}

void ImageViewer::updateActions()
{
    saveAsAct->setEnabled(!image.isNull());
    saveAct->setEnabled(!image.isNull());
    rotateAct->setEnabled(!image.isNull());
    resetAct->setEnabled(!image.isNull());
    zoomInAct->setEnabled(true);
    zoomOutAct->setEnabled(true);
    zoomByFactorAct->setEnabled(true);
}

void ImageViewer::scaleImage(double factor)
{
    Q_ASSERT(imageLabel->pixmap());
    scaleFactor *= factor;
    imageLabel->resize(scaleFactor * imageLabel->pixmap()->size());

    adjustScrollBar(scrollArea->horizontalScrollBar(), factor);
    adjustScrollBar(scrollArea->verticalScrollBar(), factor);

    zoomInAct->setEnabled(scaleFactor < 3.0);
    zoomOutAct->setEnabled(scaleFactor > 0.333);
}

void ImageViewer::adjustScrollBar(QScrollBar *scrollBar, double factor)
{
    scrollBar->setValue(int(factor * scrollBar->value()
                            + ((factor - 1) * scrollBar->pageStep()/2)));
}

void ImageViewer::mousePressEvent(QMouseEvent *e)
{
    start = imageLabel->mapFromParent(e->pos());
    mypoint = imageLabel->mapFromParent(e->pos());
    rubberBand = new QRubberBand(QRubberBand::Rectangle, imageLabel);//new rectangle band
    rubberBand->setGeometry(QRect(mypoint, imageLabel->size()));
    rubberBand->show();
}

void ImageViewer::mouseMoveEvent(QMouseEvent *e)
{
    rubberBand->setGeometry(QRect(mypoint, imageLabel->mapFromParent(e->pos())).normalized());//Area Bounding
}

void ImageViewer::mouseReleaseEvent(QMouseEvent *e)
{
    end = imageLabel->mapFromParent(e->pos());
    QMenu* contextMenu = new QMenu ( this );
    Q_CHECK_PTR ( contextMenu );
    contextMenu->addAction ( "zoom in" , this , SLOT (zoomSelect()) );
    contextMenu->addAction ( "crop" , this , SLOT (crop()) );
    contextMenu->popup( QCursor::pos() );
    contextMenu->exec ();
    delete contextMenu;
    rubberBand->hide();// hide on mouse Release
    rubberBand->clearMask();
    contextMenu = 0;
}

void ImageViewer::zoomSelection(QRect selectionRect)
{

    if(zoomInAct->isEnabled())
    {
        double rectWidth = selectionRect.width();
        double rectHeight = selectionRect.height();
        double windowWidth = window()->width();
        double windowHeight = window()->height();
        double labelWidth = imageLabel->size().width();
        double labelHeight = imageLabel->size().height();
        double factor = std::min(windowWidth / rectWidth, windowHeight / rectHeight);

        QPoint a = selectionRect.topLeft();
        imageLabel->resize(labelWidth * factor, labelHeight * factor);
        a.setX(a.x()*factor),a.setY(a.y()*factor);

        scrollArea->setUpdatesEnabled(true);
        scrollArea->horizontalScrollBar()->setUpdatesEnabled(true);
        scrollArea->verticalScrollBar()->setUpdatesEnabled(true);
        scrollArea->horizontalScrollBar()->setRange(0,imageLabel->width());
        scrollArea->horizontalScrollBar()->setValue(a.x());
        scrollArea->verticalScrollBar()->setRange(0,imageLabel->height());
        scrollArea->verticalScrollBar()->setValue(a.y());

        zoomInAct->setEnabled(scaleFactor < 3.0);
        zoomOutAct->setEnabled(scaleFactor > 0.333);
        scaleFactor *= factor;

    }
}

void ImageViewer::crop()
{
    QPixmap pixmap = imageLabel->pixmap()->copy(QRect(start, end));
    redo_stack.clear();
    setImage(pixmap.toImage());
}

void ImageViewer::rotate()
{
    int angle = QInputDialog::getInt(this, "", "Enter Angel");
    QPixmap pixmap(*imageLabel->pixmap());
    QMatrix rm;
    rm.rotate(angle);
    pixmap = pixmap.transformed(rm);
    redo_stack.clear();
    setImage(pixmap.toImage());
}

void ImageViewer::undo()
{

    if(undo_stack.size() == 2)
        undoAct->setEnabled(false);

    StackElement temp_ele = undo_stack.pop();
    redo_stack.push(temp_ele);
    StackElement cur_ele = undo_stack.pop();
    setImage(cur_ele.image);
    scaleImage(cur_ele.factor);
    redoAct->setEnabled(true);

}

void ImageViewer::redo()
{
    StackElement cur_ele = redo_stack.pop();
    setImage(cur_ele.image);
    scaleImage(cur_ele.factor);
    redoAct->setEnabled(true);
    if(redo_stack.size() == 0)
        redoAct->setEnabled(false);

}
