/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <memory>

#include <QWidget>
#include <QPainter>
#include <QTransform>
#include <QVideoSurfaceFormat>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>

#include <QDebug>
#include <QGraphicsWidget>

#include "videoitem.hpp"
class VideoPainter : public QOpenGLFunctions
{
	virtual void initializeGL()
	{
		initializeOpenGLFunctions();
	}
	QSizeF viewport_size;

public:

	void set_viewport_size(QSizeF _viewport_size)
	{
		viewport_size = _viewport_size;
		const GLdouble v_array[] =
		{
			0.0     , 0.0,
			viewport_size.width(), 0.0,
			viewport_size.width(), viewport_size.height(),

			0.0, viewport_size.height(),
		};

		if(!m_drawing_vexteres.isCreated())
			m_drawing_vexteres.create();

		m_drawing_vexteres.bind();

		m_drawing_vexteres.allocate(v_array,  sizeof(v_array));
	}

	void paint_Texture(GLuint texture)
	{
		glUseProgram(0);

		glEnable(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, texture);

		glBegin(GL_POLYGON);

		glTexCoord2d(0, 0);
		glVertex2d(0, 0);

		glTexCoord2d(1, 0);
		glVertex2d(viewport_size.width(), 0);

		glTexCoord2d(1,1);
		glVertex2d(viewport_size.width(), viewport_size.height());

		glTexCoord2d(0, 1);
		glVertex2d(0, viewport_size.height());

		glEnd();

	}

	virtual void paintGL(const QSizeF& video_size)
	{
		glEnable(GL_TEXTURE_2D);

		float brightness = 1.0;
		float contrast = 1.0;
		float saturation = 1.0;

		m_current_program->bind();

		std::shared_ptr<int> auto_cleanup;

		switch(render_type)
		{
			case render_direct_texture:
				glBindTexture(GL_TEXTURE_2D,  GLuint(*_direct_texture_id));
			break;
			case render_YUV_texture:

				// 把 设定传递给 shader 里的对应变量.
				m_program_has_yuv_shader.setUniformValue("brightness", brightness);
				m_program_has_yuv_shader.setUniformValue("contrast", contrast);
				m_program_has_yuv_shader.setUniformValue("saturation", saturation);

				// 把 shader 里的 tex0 tex1 tex2变量 和 0号 1号 2号 三个纹理缓存绑定.


				m_texture_Y->bind(0);
				m_texture_U->bind(1);
				m_texture_V->bind(2);

				auto_cleanup.reset((int*)nullptr, [this](int*)
				{
					m_texture_Y->release();
					m_texture_U->release();
					m_texture_V->release();

				});
		}


		m_current_program->setUniformValue("video_window_size", QVector2D(viewport_size.width(), viewport_size.height()));
		m_current_program->setUniformValue("texture_size", QVector2D(video_size.width(), video_size.height()));
		m_current_program->setUniformValue("tex0", 0u);
		m_current_program->setUniformValue("tex1", 1u);
		m_current_program->setUniformValue("tex2", 2u);

 		m_drawing_vexteres.bind();

		m_current_program->enableAttributeArray(0);
		glVertexAttribPointer(0, 2, GL_DOUBLE, GL_FALSE, 0, 0);

		glDrawArrays(GL_POLYGON, 0, 4);

		m_drawing_vexteres.release();
// 		auto_cleanup.reset();
		m_current_program->release();
	}

public:

	VideoPainter(VideoItem* parent)
		: m_drawing_vexteres(QOpenGLBuffer::VertexBuffer)
	{
		initializeGL();

		m_drawing_vexteres.setUsagePattern(QOpenGLBuffer::DynamicDraw);

		m_program_has_yuv_shader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glsl/videowindow.vert");
		m_program_has_yuv_shader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glsl/yuv.frag");

// gl_Vertex 	0
// gl_Normal 	2
// gl_Color 	3
// gl_SecondaryColor 	4
// gl_FogCoord 	5
// gl_MultiTexCoord0 	8
// gl_MultiTexCoord1 	9
// gl_MultiTexCoord2 	10
// gl_MultiTexCoord3 	11
// gl_MultiTexCoord4 	12
// gl_MultiTexCoord5 	13
// gl_MultiTexCoord6 	14
// gl_MultiTexCoord7 	15
//
		m_program_has_yuv_shader.bindAttributeLocation("gl_Vertex", 0);
		m_program_has_yuv_shader.bindAttributeLocation("gl_MultiTexCoord0", 8);

		m_program_has_yuv_shader.link();


		m_program_only_vertex_shader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glsl/videowindow.vert");
		m_program_only_vertex_shader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glsl/passthru.frag");
		m_program_only_vertex_shader.bindAttributeLocation("gl_Vertex", 0);
		m_program_only_vertex_shader.bindAttributeLocation("gl_MultiTexCoord0", 8);

		m_program_only_vertex_shader.link();
	}

	~VideoPainter()
	{
		if (m_texture_Y)
			m_texture_Y->destroy();
		if (m_texture_U)
			m_texture_U->destroy();
		if (m_texture_V)
			m_texture_V->destroy();

		m_program_has_yuv_shader.removeAllShaders();
		m_program_only_vertex_shader.removeAllShaders();
	}


	void update_yuv420p_texture(const QVideoFrame& newframe)
	{
		if (!m_texture_Y)
			m_texture_Y.reset(new QOpenGLTexture(QOpenGLTexture::Target2D));
		if (!m_texture_U)
			m_texture_U.reset(new QOpenGLTexture(QOpenGLTexture::Target2D));
		if (!m_texture_V)
			m_texture_V.reset(new QOpenGLTexture(QOpenGLTexture::Target2D));

		auto vsize = newframe.size();
		// update texture fome newframe
		if (m_texture_Y->isCreated())
			m_texture_Y->destroy();

		m_texture_Y->setSize(newframe.bytesPerLine(0), vsize.height(), 0);
		m_texture_Y->setFormat(QOpenGLTexture::R8_UNorm);
		m_texture_Y->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
		m_texture_Y->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);
		m_texture_Y->allocateStorage();
		m_texture_Y->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, newframe.bits(0));

		if(m_texture_U->isCreated())
			m_texture_U->destroy();

		m_texture_U->setSize(newframe.bytesPerLine(1), vsize.height()/2, 0);
		m_texture_U->setFormat( QOpenGLTexture::R8_UNorm);
		m_texture_U->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
		m_texture_U->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);
		m_texture_U->allocateStorage();
		m_texture_U->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, newframe.bits(1));

		if(m_texture_V->isCreated())
			m_texture_V->destroy();

		m_texture_V->setSize(newframe.bytesPerLine(2), vsize.height()/2, 0);
		m_texture_V->setFormat(QOpenGLTexture::R8_UNorm);
		m_texture_V->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
		m_texture_V->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);
		m_texture_V->allocateStorage();
		m_texture_V->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, newframe.bits(2));
	}

	void update_direct_texture(GLuint textureid)
	{
		_direct_texture_id.reset(new GLuint(textureid), [this](GLuint * p)
		{
			glDeleteTextures(1, p);
		});
	}

	void update_texture(const QVideoFrame& newframe)
	{
		if (newframe.handleType() == QAbstractVideoBuffer::NoHandle && newframe.pixelFormat() == QVideoFrame::Format_YUV420P)
		{
			render_type = render_YUV_texture;
			update_yuv420p_texture(newframe);

			if(!m_drawing_vexteres.isCreated())
				m_drawing_vexteres.create();

			m_current_program = & m_program_has_yuv_shader;
		}
		else if (newframe.handleType() == QAbstractVideoBuffer::GLTextureHandle)
		{
			// 已经是 texture 啦？

			render_type = render_direct_texture;

			update_direct_texture(newframe.handle().toUInt());

			m_current_program = & m_program_only_vertex_shader;
		}
	}

	enum { render_YUV_texture, render_direct_texture } render_type;


	QScopedPointer<QOpenGLTexture> m_texture_rgb;

	std::shared_ptr<GLuint> _direct_texture_id;

	QScopedPointer<QOpenGLTexture> m_texture_Y;
	QScopedPointer<QOpenGLTexture> m_texture_U;
	QScopedPointer<QOpenGLTexture> m_texture_V;

	QOpenGLShaderProgram m_program_only_vertex_shader;
	QOpenGLShaderProgram m_program_has_yuv_shader;

	QOpenGLShaderProgram* m_current_program;

	QOpenGLBuffer m_drawing_vexteres;
};

VideoItem::VideoItem(QGraphicsItem *parent)
	: QGraphicsItem(parent)
	, imageFormat(QImage::Format_Invalid)
	, framePainted(false)
{
	m_painter = nullptr;
	updatePaintDevice = true;
}

VideoItem::~VideoItem()
{
}

QRectF VideoItem::boundingRect() const
{
	if( my_size.isValid())
		return QRectF(QPointF(0,0), my_size);
    return QRectF(QPointF(0,0), surfaceFormat().sizeHint());
}

void VideoItem::viewportDestroyed()
{
	delete m_painter;

	m_painter = nullptr;

	updatePaintDevice = true;
}

void VideoItem::resize(QSizeF newsize)
{
	my_size = newsize;
	if (m_painter)
		m_painter->set_viewport_size(my_size);
}

void VideoItem::paintImage(QPainter* painter)
{
	painter->drawImage(boundingRect(), QImage(
			currentFrame.bits(),
			imageSize.width(),
			imageSize.height(),
			imageFormat));
}

void VideoItem::paintGL(QPainter* painter)
{
	Q_ASSERT( QOpenGLContext::currentContext() );

	painter->beginNativePainting();

	auto vsize = currentFrame.size();

	if ( need_update_gltexture && currentFrame.map(QAbstractVideoBuffer::ReadOnly))
	{
		m_painter->update_texture(currentFrame);
		currentFrame.unmap();

		need_update_gltexture = false;
	}

	m_painter->paintGL(vsize);
	painter->endNativePainting();
}

void VideoItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	QMutexLocker l(&m_render_lock);
	if(!currentFrame.isValid())
		return;

	if (updatePaintDevice && !m_painter)
	{
		updatePaintDevice = false;

		m_painter = new VideoPainter(this);
        if (widget)
             connect(widget, SIGNAL(destroyed()), this, SLOT(viewportDestroyed()));

		m_painter->set_viewport_size(my_size);
	}

	const QTransform oldTransform = painter->transform();

	bool use_gl_painter = false;

	if (currentFrame.handleType() == QAbstractVideoBuffer::GLTextureHandle)
	{
		use_gl_painter = true;
	}
	else if (currentFrame.pixelFormat() == QVideoFrame::Format_YUV420P)
	{
		use_gl_painter = true;
	}

	if (surfaceFormat().scanLineDirection() == QVideoSurfaceFormat::BottomToTop)
	{
		painter->scale(1, -1);
		painter->translate(0, - my_size.height());
	}

	if (!use_gl_painter)
	{
		if (currentFrame.map(QAbstractVideoBuffer::ReadOnly))
		{
			paintImage(painter);
			currentFrame.unmap();
		}
	}
	else
	{
		paintGL(painter);
	}
	painter->setTransform(oldTransform);

	framePainted = true;
}

QList<QVideoFrame::PixelFormat> VideoItem::supportedPixelFormats(
        QAbstractVideoBuffer::HandleType handleType) const
{
	if (handleType == QAbstractVideoBuffer::NoHandle)
	{
		return QList<QVideoFrame::PixelFormat>()
				<< QVideoFrame::Format_RGB32
				<< QVideoFrame::Format_ARGB32
				<< QVideoFrame::Format_ARGB32_Premultiplied
				<< QVideoFrame::Format_RGB565
				<< QVideoFrame::Format_RGB555
				<< QVideoFrame::Format_YUV420P;
	}
	else if(handleType == QAbstractVideoBuffer::GLTextureHandle)
	{
		return QList<QVideoFrame::PixelFormat>()
				<< QVideoFrame::Format_RGB32
				<< QVideoFrame::Format_RGB24
				<< QVideoFrame::Format_ARGB32
				<< QVideoFrame::Format_ARGB32_Premultiplied
				<< QVideoFrame::Format_RGB565
				<< QVideoFrame::Format_BGR24
				<< QVideoFrame::Format_BGR32
				<< QVideoFrame::Format_RGB555;
	}
	else
	{
		return QList<QVideoFrame::PixelFormat>();
	}
}

bool VideoItem::start(const QVideoSurfaceFormat &format)
{
	if (isFormatSupported(format))
	{
		imageFormat = QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat());
		imageSize = format.frameSize();
		framePainted = true;

		qDebug() << format.pixelFormat();

		QAbstractVideoSurface::start(format);

		prepareGeometryChange();

		return true;
	} else {
		return false;
	}
}

void VideoItem::stop()
{
	currentFrame = QVideoFrame();
	framePainted = false;

	QAbstractVideoSurface::stop();
}

bool VideoItem::present(const QVideoFrame &frame)
{
	QMutexLocker l(&m_render_lock);

	if (frame.isValid())
		currentFrame = frame;

	need_update_gltexture = true;

	if (!framePainted) {
		if (!QAbstractVideoSurface::isActive())
			setError(QAbstractVideoSurface::StoppedError);
		return false;
	} else {
		framePainted = false;

		update();

		return true;
	}
}

// #include "moc_videoitem.cpp"