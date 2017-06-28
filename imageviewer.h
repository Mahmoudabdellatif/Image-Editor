#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QMainWindow>
#include <QImage>
#include <QRubberBand>
#include <QStack>
#include "stackelement.h"

class QAction;
class QLabel;
class QMenu;
class QScrollArea;
class QScrollBar;

class ImageViewer : public QMainWindow
{
    Q_OBJECT

public:
    ImageViewer();
    bool loadFile(const QString &);

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

private slots:
    void open();
    void save();
    void saveAs();
    void zoomIn();
    void zoomOut();
    void zoomByFactor();
    void zoomSelect();
    void crop();
    void rotate();
    void undo();
    void redo();
    void reset();

private:
    void createActions();
    void createMenus();
    void updateActions();
    bool saveFile(const QString &filename);
    void setImage(const QImage &newImage);
    void scaleImage(double factor);
    void adjustScrollBar(QScrollBar *scrollBar, double factor);
    void zoomSelection(QRect selectionRect);
    void setEle();

    QImage image;
    QLabel *imageLabel;
    QScrollArea *scrollArea;
    double scaleFactor;

    QAction *saveAct;
    QAction *saveAsAct;
    QAction *zoomInAct;
    QAction *zoomOutAct;
    QAction *zoomByFactorAct;
    QAction *undoAct;
    QAction *redoAct;
    QAction *rotateAct;
    QAction *resetAct;

    QRubberBand *rubberBand;
    QPoint mypoint;
    QPoint start;
    QPoint end;
    QString fileName;

    QStack<StackElement> undo_stack;
    QStack<StackElement> redo_stack;

};

#endif
