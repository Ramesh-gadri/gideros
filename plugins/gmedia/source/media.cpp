#include <media.h>
#include <QApplication>
#include <QMediaDevices>
#include <QMediaCaptureSession>
#include <QMediaPlayer>
#include <QScreen>
#include <QVideoWidget>
#include <QImageCapture>
#include <QString>
#include <QFileDialog>
#include <QPixmap>
#include <QDateTime>
#include <QObject>

	GMEDIA::GMEDIA()
	{
		gid_ = g_NextId();
		camera=NULL;
	}

	GMEDIA::~GMEDIA()
	{

		gevent_RemoveEventsWithGid(gid_);
    }
	
	bool GMEDIA::isCameraAvailable()
	{
        if (QMediaDevices::videoInputs().count() > 0)
            return true;
        else
            return false;
    }
	
	void GMEDIA::takePicture()
	{
		if (!camera)
		{
            const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
            for (const QCameraDevice &cameraDevice : cameras) {
               camera = new QCamera(cameraDevice);
               break;
            }
		}
        if (!camera) return;

        QMediaCaptureSession captureSession;
        captureSession.setCamera(camera);
        QImageCapture imageCapture;
        captureSession.setImageCapture(&imageCapture);

        camera->start();

        //on shutter button pressed
        imageCapture.capture();


        QDateTime createdDate = QDateTime::currentDateTime();
        QString format = "jpg";
        QString fileName = getAppPath() + "/" + createdDate.toString("yyyyMMdd_HHmmss") + "_gideros." + format;

        connect(&imageCapture, SIGNAL(imageSaved(int, const QString &)),
                         this,  SLOT(pictureSaved(int, const QString &)));
        imageCapture.setFileFormat(QImageCapture::JPEG);
        imageCapture.captureToFile(fileName);
	}

    void GMEDIA::takeScreenshot()
    {
        QDateTime createdDate = QDateTime::currentDateTime();
        QWidget *widget = QApplication::activeWindow();
        QPixmap originalPixmap = widget->screen()->grabWindow(widget->winId());
        QString format = "png";
        QString fileName = getAppPath() + "/" + createdDate.toString("yyyyMMdd_HHmmss") + "_gideros." + format;
        originalPixmap.save(fileName, format.toStdString().c_str());
        onMediaReceived(fileName.toStdString().c_str());
    }
	
	void GMEDIA::getPicture()
	{
        QString fileName = QFileDialog::getOpenFileName(0, QObject::tr("Open File"),"",QObject::tr("Images (*.png *.jpeg *.jpg)"));
        if(fileName.isNull())
            onMediaCanceled();
        else{
            QFileInfo info(fileName);
            QDateTime createdDate = QDateTime::currentDateTime();
            QString destName = getAppPath() + "/" + createdDate.toString("yyyyMMdd_HHmmss") + "_gideros." + info.suffix();
            if(QFile::exists(destName))
            {
                QFile::remove(destName);
            }
            QFile::copy(fileName, destName);
            onMediaReceived(destName.toStdString().c_str());
        }
	}
	
	void GMEDIA::getFile(const char* extensions, const char* initialPath)
	{
		QFileDialog *dialog = new QFileDialog(0, QObject::tr("Open File"),initialPath,QObject::tr(extensions));
		dialog->setFileMode(QFileDialog::ExistingFile);
		dialog->setAcceptMode(QFileDialog::AcceptOpen);

		connect(dialog, &QFileDialog::finished, [this, dialog](int result) {
			 if (result) {
				 QString fileName = dialog->selectedFiles().at(0);
		         onMediaReceived(fileName.toStdString().c_str());
			 }
			 else
				 onMediaCanceled();
			 dialog->deleteLater();
		});
		dialog->show();
	}

    void GMEDIA::savePicture(const char* path)
	{
        QFileInfo info(path);
        QString format = info.suffix();
        QString initialPath = getAppPath() + "/" + info.fileName();

        QString fileName = QFileDialog::getSaveFileName(0, QObject::tr("Save As"),
                                        initialPath,
                                        QObject::tr("%1 Files (*.%2);;All Files (*)")
                                        .arg(format.toUpper())
                                        .arg(format));
        if (!fileName.isEmpty())
        {
            if(QFile::exists(fileName))
            {
                QFile::remove(fileName);
            }

            QFile::copy(path, fileName);
        }
	}

void GMEDIA::saveFile(const char* path, const char* initialPath)
	{
        QFileInfo info(path);
        QString format = info.suffix();        

		QFileDialog *dialog = new QFileDialog(0, QObject::tr("Save As"),initialPath,
                QObject::tr("%1 Files (*.%2);;All Files (*)")
                .arg(format.toUpper())
                .arg(format));
		dialog->setFileMode(QFileDialog::AnyFile);
		dialog->setAcceptMode(QFileDialog::AcceptSave);
		connect(dialog, &QFileDialog::finished, [this, dialog, path](int result) {
			 if (result) {
				 QString fileName = dialog->selectedFiles().at(0);
			        if (!fileName.isEmpty())
			        {
			            if(QFile::exists(fileName))
			            {
			                QFile::remove(fileName);
			            }

			            QFile::copy(path, fileName);
			        }
			 }
			 dialog->deleteLater();
		});
		dialog->show();
	}
							
    void GMEDIA::playVideo(const char* path, bool force)
    {
        G_UNUSED(force);
        QMediaPlayer* player = new QMediaPlayer;

        player->setSource(QUrl(path));

        QVideoWidget* videoWidget = new QVideoWidget;
        player->setVideoOutput(videoWidget);

        videoWidget->show();
        player->play();
    }

    QString GMEDIA::getAppPath(){
        QDir dir = QDir::temp();
        dir.mkdir("gideros");
        dir.cd("gideros");
        dir.mkdir("mediafiles");
        dir.cd("mediafiles");
        return dir.absolutePath();
    }
	
    void GMEDIA::onMediaReceived(const char* path)
	{

		gmedia_ReceivedEvent *event = (gmedia_ReceivedEvent*)gevent_CreateEventStruct1(
			sizeof(gmedia_ReceivedEvent),
            offsetof(gmedia_ReceivedEvent, path), path);
		gevent_EnqueueEvent(gid_, callback_s, GMEDIA_RECEIVED_EVENT, event, 1, this);
	}

    void GMEDIA::onMediaCompleted()
    {
        gevent_EnqueueEvent(gid_, callback_s, GMEDIA_COMPLETED_EVENT, NULL, 1, this);
    }
	
	void GMEDIA::onMediaCanceled()
    {
		gevent_EnqueueEvent(gid_, callback_s, GMEDIA_CANCELED_EVENT, NULL, 1, this);
	}
	
	g_id GMEDIA::addCallback(gevent_Callback callback, void *udata)
	{
		return callbackList_.addCallback(callback, udata);
	}
	void GMEDIA::removeCallback(gevent_Callback callback, void *udata)
	{
		callbackList_.removeCallback(callback, udata);
	}
	void GMEDIA::removeCallbackWithGid(g_id gid)
	{
		callbackList_.removeCallbackWithGid(gid);
	}

	void GMEDIA::callback_s(int type, void *event, void *udata)
	{
		((GMEDIA*)udata)->callback(type, event);
	}

	void GMEDIA::callback(int type, void *event)
	{
		callbackList_.dispatchEvent(type, event);
	}

    void GMEDIA::pictureSaved(int id, const QString & fileName)
    {
        //on shutter button released
        G_UNUSED(id);
        camera->stop();
        onMediaReceived(fileName.toStdString().c_str());
    }


static GMEDIA *s_gmedia = NULL;

extern "C" {

void gmedia_init()
{
    s_gmedia = new GMEDIA;
}

void gmedia_cleanup()
{
    delete s_gmedia;
    s_gmedia = NULL;
}

int gmedia_isCameraAvailable()
{
    return s_gmedia->isCameraAvailable();
}

void gmedia_takePicture(){
    s_gmedia->takePicture();
}

void gmedia_takeScreenshot(){
    s_gmedia->takeScreenshot();
}

void gmedia_getPicture(){
    s_gmedia->getPicture();
}

void gmedia_getFile(const char* extensions, const char* initialPath){
    s_gmedia->getFile(extensions, initialPath);
}
	
void gmedia_savePicture(const char* path){
    s_gmedia->savePicture(path);
}

void gmedia_saveFile(const char* path, const char* initialPath){
    s_gmedia->saveFile(path, initialPath);
}
	
void gmedia_playVideo(const char* path, int force){
    s_gmedia->playVideo(path, force);
}

g_id gmedia_addCallback(gevent_Callback callback, void *udata)
{
    return s_gmedia->addCallback(callback, udata);
}

void gmedia_removeCallback(gevent_Callback callback, void *udata)
{
    s_gmedia->removeCallback(callback, udata);
}

void gmedia_removeCallbackWithGid(g_id gid)
{
    s_gmedia->removeCallbackWithGid(gid);
}

}
