#include <ghttp.h>
#include <ghttp-qt.h>

static bool sslErrorsIgnore=false;

HTTPManager::HTTPManager()
{
    manager_ = new QNetworkAccessManager();
    connect(manager_, SIGNAL(finished(QNetworkReply*)),
            this,		SLOT(finished(QNetworkReply*)));
}

HTTPManager::~HTTPManager()
{
    CloseAll();
    delete manager_;
}

void HTTPManager::Close(g_id id)
{
    std::map<QNetworkReply*, NetworkReply>::iterator iter = map_.begin(), e = map_.end();
    for (; iter != e; ++iter)
    {
        if (iter->second.id == id)
        {
            iter->first->deleteLater();
            map_.erase(iter);
            break;
        }
    }

    gevent_RemoveEventsWithGid(id);
}

void HTTPManager::CloseAll()
{
    while (!map_.empty())
        Close(map_.begin()->second.id);
}

g_id HTTPManager::Get(const char *url, const ghttp_Header *header, bool streaming, gevent_Callback callback, void *udata)
{
    QNetworkRequest request(QUrl::fromEncoded(url));

    request.setRawHeader("User-Agent", "Gideros");

    if (header)
        for (; header->name; ++header)
            request.setRawHeader(QByteArray(header->name), QByteArray(header->value));

    QNetworkReply *reply = manager_->get(request);
    if (sslErrorsIgnore)
    	reply->ignoreSslErrors();

    connect(reply, SIGNAL(downloadProgress(qint64, qint64)),
            this,	 SLOT(downloadProgress(qint64, qint64)));

    NetworkReply reply2;
    reply2.id = g_NextId();
    reply2.callback = callback;
    reply2.udata = udata;
    reply2.streaming=streaming;
    reply2.started=false;
    map_[reply] = reply2;

    return reply2.id;
}


g_id HTTPManager::Post(const char *url, const ghttp_Header *header, const void *data, size_t size, bool streaming, gevent_Callback callback, void *udata)
{
    QNetworkRequest request(QUrl::fromEncoded(url));

    request.setRawHeader("User-Agent", "Gideros");

    if (header)
        for (; header->name; ++header)
            request.setRawHeader(QByteArray(header->name), QByteArray(header->value));

    QNetworkReply *reply = manager_->post(request, QByteArray((char*)data, size));
    if (sslErrorsIgnore)
    	reply->ignoreSslErrors();

    connect(reply, SIGNAL(downloadProgress(qint64, qint64)),
            this,	 SLOT(downloadProgress(qint64, qint64)));

    NetworkReply reply2;
    reply2.id = g_NextId();
    reply2.callback = callback;
    reply2.udata = udata;
    reply2.streaming=streaming;
    reply2.started=false;
    map_[reply] = reply2;

    return reply2.id;
}

g_id HTTPManager::Delete(const char *url, const ghttp_Header *header, bool streaming, gevent_Callback callback, void *udata)
{
    QNetworkRequest request(QUrl::fromEncoded(url));

    request.setRawHeader("User-Agent", "Gideros");

    if (header)
        for (; header->name; ++header)
            request.setRawHeader(QByteArray(header->name), QByteArray(header->value));

    QNetworkReply *reply = manager_->deleteResource(request);
    if (sslErrorsIgnore)
    	reply->ignoreSslErrors();

    connect(reply, SIGNAL(downloadProgress(qint64, qint64)),
            this,	 SLOT(downloadProgress(qint64, qint64)));

    NetworkReply reply2;
    reply2.id = g_NextId();
    reply2.callback = callback;
    reply2.udata = udata;
    reply2.streaming=streaming;
    reply2.started=false;
    map_[reply] = reply2;

    return reply2.id;
}

g_id HTTPManager::Put(const char *url, const ghttp_Header *header, const void *data, size_t size, bool streaming, gevent_Callback callback, void *udata)
{
    QNetworkRequest request(QUrl::fromEncoded(url));

    request.setRawHeader("User-Agent", "Gideros");

    if (header)
        for (; header->name; ++header)
            request.setRawHeader(QByteArray(header->name), QByteArray(header->value));

    QNetworkReply *reply = manager_->put(request, QByteArray((char*)data, size));
    if (sslErrorsIgnore)
    	reply->ignoreSslErrors();

    connect(reply, SIGNAL(downloadProgress(qint64, qint64)),
            this,	 SLOT(downloadProgress(qint64, qint64)));

    NetworkReply reply2;
    reply2.id = g_NextId();
    reply2.callback = callback;
    reply2.udata = udata;
    reply2.streaming=streaming;
    reply2.started=false;
    map_[reply] = reply2;

    return reply2.id;
}

void HTTPManager::finished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (map_.find(reply) == map_.end())
        return;

    QVariant statusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute );
/*
    if (reply->error() != QNetworkReply::NoError
        // a web page that returns 403/404 can still have content
        && reply->error() != QNetworkReply::ContentOperationNotPermittedError
        && reply->error() != QNetworkReply::ContentNotFoundError
        && reply->error() != QNetworkReply::UnknownContentError)*/
    if ( !statusCode.isValid() )
    {
        NetworkReply reply2 = map_[reply];

        ghttp_ErrorEvent *event = (ghttp_ErrorEvent*)malloc(sizeof(ghttp_ErrorEvent));

        gevent_EnqueueEvent(reply2.id, reply2.callback, GHTTP_ERROR_EVENT, event, 1, reply2.udata);
    }
    else
    {
        int status = statusCode.toInt();

        QByteArray bytes = reply->readAll();
        QList<QNetworkReply::RawHeaderPair> headers=reply->rawHeaderPairs();
        int hdrCount=headers.count();
        int hdrSize=0;
        foreach (QNetworkReply::RawHeaderPair h, headers)
        {
            hdrSize+=h.first.size();
            hdrSize+=h.second.size();
           	hdrSize+=2;
        }

        NetworkReply reply2 = map_[reply];

        ghttp_ResponseEvent *event = (ghttp_ResponseEvent*)malloc(sizeof(ghttp_ResponseEvent)  + sizeof(ghttp_Header)*hdrCount + bytes.size() + hdrSize);

        event->data = (char*)event + sizeof(ghttp_ResponseEvent) + sizeof(ghttp_Header)*hdrCount;
        memcpy(event->data, bytes.constData(), bytes.size());
        event->size = bytes.size();

        event->httpStatusCode = status;

		int hdrn=0;
		char *hdrData=(char *)(event->data)+bytes.size();
        foreach (QNetworkReply::RawHeaderPair h, headers)
        {
            int ds=h.first.size();
            memcpy(hdrData,h.first.data(),ds);
	 		event->headers[hdrn].name=hdrData;
        	hdrData+=ds;
        	*(hdrData++)=0;
            ds=h.second.size();
            memcpy(hdrData,h.second.data(),ds);
	 		event->headers[hdrn].value=hdrData;
        	hdrData+=ds;
        	*(hdrData++)=0;
			hdrn++;
        }
		event->headers[hdrn].name=NULL;
		event->headers[hdrn].value=NULL;

        gevent_EnqueueEvent(reply2.id, reply2.callback, GHTTP_RESPONSE_EVENT, event, 1, reply2.udata);
    }

    map_.erase(reply);
}

void HTTPManager::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    if (map_.find(reply) == map_.end())
        return;

    NetworkReply reply2 = map_[reply];

    ghttp_ProgressEvent* event;
    if (reply2.streaming) {
    	if (!reply2.started) {
			reply2.started=true;
			map_[reply]=reply2;
		    QVariant statusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute );
			int status = statusCode.toInt();

			QList<QNetworkReply::RawHeaderPair> headers=reply->rawHeaderPairs();
			int hdrCount=headers.count();
			int hdrSize=0;
            foreach (QNetworkReply::RawHeaderPair h, headers)
            {
                hdrSize+=h.first.size();
                hdrSize+=h.second.size();
				hdrSize+=2;
			}

			ghttp_ResponseEvent *event = (ghttp_ResponseEvent*)malloc(sizeof(ghttp_ResponseEvent)  + sizeof(ghttp_Header)*hdrCount + hdrSize);

			event->data = NULL;
			event->size = 0;

			event->httpStatusCode = status;

			int hdrn=0;
			char *hdrData=(char *)(event+1);
            foreach (QNetworkReply::RawHeaderPair h, headers)
            {
                int ds=h.first.size();
                memcpy(hdrData,h.first.data(),ds);
				event->headers[hdrn].name=hdrData;
				hdrData+=ds;
				*(hdrData++)=0;
                ds=h.second.size();
                memcpy(hdrData,h.second.data(),ds);
				event->headers[hdrn].value=hdrData;
				hdrData+=ds;
				*(hdrData++)=0;
				hdrn++;
			}
			event->headers[hdrn].name=NULL;
			event->headers[hdrn].value=NULL;

			gevent_EnqueueEvent(reply2.id, reply2.callback, GHTTP_HEADER_EVENT, event, 1, reply2.udata);
		}
		QByteArray bytes = reply->readAll();
        event = (ghttp_ProgressEvent*)malloc(sizeof(ghttp_ProgressEvent)+ bytes.size());
		event->chunk = (char*)event + sizeof(ghttp_ProgressEvent);
		memcpy(event->chunk, bytes.constData(), bytes.size());
		event->chunkSize = bytes.size();
    }
    else {
        event = (ghttp_ProgressEvent*)malloc(sizeof(ghttp_ProgressEvent));
    	event->chunk=NULL;
    	event->chunkSize=0;
    }
    event->bytesLoaded = bytesReceived;
    event->bytesTotal = bytesTotal;

    gevent_EnqueueEvent(reply2.id, reply2.callback, GHTTP_PROGRESS_EVENT, event, 1, reply2.udata);
}

static HTTPManager* s_manager = NULL;

extern "C" {

void ghttp_IgnoreSSLErrors()
{
	sslErrorsIgnore=true;
}

void ghttp_SetProxy(const char */* host */, int /* port */, const char */* user */, const char */* pass */)
{
}

void ghttp_Init()
{
    s_manager = new HTTPManager();
}

void ghttp_Cleanup()
{
    delete s_manager;
    s_manager = NULL;
}

g_id ghttp_Get(const char* url, const ghttp_Header *header, int streaming, gevent_Callback callback, void* udata)
{
    return s_manager->Get(url, header, streaming, callback, udata);
}

g_id ghttp_Post(const char* url, const ghttp_Header *header, const void* data, size_t size, int streaming, gevent_Callback callback, void* udata)
{
    return s_manager->Post(url, header, data, size, streaming, callback, udata);
}

g_id ghttp_Delete(const char* url, const ghttp_Header *header, int streaming, gevent_Callback callback, void* udata)
{
    return s_manager->Delete(url, header, streaming, callback, udata);
}

g_id ghttp_Put(const char* url, const ghttp_Header *header, const void* data, size_t size, int streaming, gevent_Callback callback, void* udata)
{
    return s_manager->Put(url, header, data, size, streaming, callback, udata);
}

void ghttp_Close(g_id id)
{
    s_manager->Close(id);
}

void ghttp_CloseAll()
{
    s_manager->CloseAll();
}

}
