/****************************************************************************
* MeshLab                                                           o o     *
* An extendible mesh processor                                    o     o   *
*                                                                _   O  _   *
* Copyright(C) 2005, 2006                                          \/)\/    *
* Visual Computing Lab                                            /\/|      *
* ISTI - Italian National Research Council                           |      *
*                                                                    \      *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
*                                                                           *
****************************************************************************/
/****************************************************************************
History

$Log$
Revision 1.107  2006/10/26 12:07:51  corsini
add lighting properties customization

Revision 1.106  2006/10/22 21:36:14  cignoni
Corrected bug per face color (now the entr y is enabled only if the mesh has the perface color)

Revision 1.105  2006/10/12 06:45:48  cignoni
Updated access to mm mask and the choice of the color mode according to the filter class

Revision 1.104  2006/07/08 06:37:47  cignoni
Many small bugs correction (esc crash, info in about, obj loading progress,fullscreen es)

Revision 1.103  2006/06/27 08:07:42  cignoni
Restructured plugins interface for simplifying the server

Revision 1.102  2006/06/19 15:17:19  cignoni
Dirty flag bug, Busy Bug, Cleaning of degenerate faces on loading.

Revision 1.101  2006/06/18 21:27:49  cignoni
Progress bar redesigned, now integrated in the workspace window

Revision 1.100  2006/06/16 01:26:07  cignoni
Added Initial Filter Script Dialog

Revision 1.99  2006/06/15 13:05:57  cignoni
added Filter History Dialogs

Revision 1.98  2006/06/13 13:50:01  cignoni
Cleaned FPS management

Revision 1.97  2006/06/12 15:21:03  cignoni
toggle between last editing mode

Revision 1.96  2006/06/07 08:49:25  cignoni
Disable rendering during processing and loading

Revision 1.95  2006/05/26 04:09:52  cignoni
Still debugging 0.7

Revision 1.94  2006/05/25 04:57:45  cignoni
Major 0.7 release. A lot of things changed. Colorize interface gone away, Editing and selection start to work.
Optional data really working. Clustering decimation totally rewrote. History start to work. Filters organized in classes.

Revision 1.93  2006/03/07 10:47:50  cignoni
Better mask management during io

Revision 1.92  2006/02/25 13:43:39  ggangemi
Action "None" is now exported from MeshRenderPlugin

Revision 1.91  2006/02/24 08:21:00  cignoni
yet another attempt to solve the QProgressDialog issue. Now trying with qt->reset.

Revision 1.90  2006/02/22 10:20:09  cignoni
Changed progressbar->hide  into close to avoid 100% cpu use.

Revision 1.89  2006/02/21 17:25:57  ggangemi
RenderMode is now passed to MeshRenderInterface::Init()

Revision 1.88  2006/02/17 11:17:23  glvertex
- Moved closeAction in FileMenu
- Minor changes

Revision 1.87  2006/02/16 15:26:58  glvertex
Solved some minimizing/restore bugs

Revision 1.86  2006/02/13 16:18:04  cignoni
Minor edits.
****************************************************************************/


#include <QtGui>
#include <QToolBar>
#include <QProgressBar>


#include "meshmodel.h"
#include "interfaces.h"
#include "mainwindow.h"
#include "glarea.h"
#include "plugindialog.h"
#include "filterScriptDialog.h"
#include "customDialog.h"
#include "lightingDialog.h"
#include "saveSnapshotDialog.h"
#include "ui_aboutDialog.h"
#include "savemaskexporter.h"
#include "plugin_support.h"

#include <wrap/io_trimesh/io_mask.h>
#include <vcg/complex/trimesh/update/normal.h>
#include <vcg/complex/trimesh/clean.h>


void MainWindow::updateRecentFileActions()
{
	QSettings settings("Recent Files");
	QStringList files = settings.value("recentFileList").toStringList();
	int numRecentFiles = qMin(files.size(), (int)MAXRECENTFILES);
	for (int i = 0; i < numRecentFiles; ++i) {
		QString text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
		recentFileActs[i]->setText(text);
		recentFileActs[i]->setData(files[i]);
		recentFileActs[i]->setVisible(true);
	}
	for (int j = numRecentFiles; j < MAXRECENTFILES; ++j)	recentFileActs[j]->setVisible(false);
	separatorAct->setVisible(numRecentFiles > 0);
}

void MainWindow::updateWindowMenu()
{
	windowsMenu->clear();
	windowsMenu->addAction(closeAllAct);
	windowsMenu->addSeparator();
	windowsMenu->addAction(windowsTileAct);
	windowsMenu->addAction(windowsCascadeAct);
	windowsMenu->addAction(windowsNextAct);
	windowsNextAct->setEnabled(workspace->windowList().size()>1);

	QWidgetList windows = workspace->windowList();

	if(windows.size() > 0)
			windowsMenu->addSeparator();

	int i=0;
	foreach(QWidget *w,windows)
	{
		QString text = tr("&%1. %2").arg(i+1).arg(QFileInfo(w->windowTitle()).fileName());
		QAction *action  = windowsMenu->addAction(text);
		action->setCheckable(true);
		action->setChecked(w == workspace->activeWindow());
		// Connect the signal to activate the selected window
		connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
		windowMapper->setMapping(action, w);
		++i;
	}
}

void MainWindow::setColorMode(QAction *qa)
{
	if(qa->text() == tr("&None"))					GLA()->setColorMode(GLW::CMNone);
	if(qa->text() == tr("Per &Vertex"))		GLA()->setColorMode(GLW::CMPerVert);
	if(qa->text() == tr("Per &Face"))			GLA()->setColorMode(GLW::CMPerFace);
}

void MainWindow::updateMenus()
{
	bool active = (bool) !workspace->windowList().empty() && workspace->activeWindow();
	closeAct->setEnabled(active);
	reloadAct->setEnabled(active);
	saveAsAct->setEnabled(active);
	saveSnapshotAct->setEnabled(active);
	filterMenu->setEnabled(active && !filterMenu->actions().isEmpty());
	editMenu->setEnabled(active && !editMenu->actions().isEmpty());
	renderMenu->setEnabled(active);
	fullScreenAct->setEnabled(active);
	trackBallMenu->setEnabled(active);
	logMenu->setEnabled(active);
  windowsMenu->setEnabled(active);
	preferencesMenu->setEnabled(active);
	
	renderToolBar->setEnabled(active);
	
	showToolbarRenderAct->setChecked(renderToolBar->isVisible());
	showToolbarStandardAct->setChecked(mainToolBar->isVisible());
	if(active){
		const RenderMode &rm=GLA()->getCurrentRenderMode();
		switch (rm.drawMode) {
			case GLW::DMBox:				renderBboxAct->setChecked(true);                break;
			case GLW::DMPoints:			renderModePointsAct->setChecked(true);      		break;
			case GLW::DMWire: 			renderModeWireAct->setChecked(true);      			break;
			case GLW::DMFlat:				renderModeFlatAct->setChecked(true);    				break;
			case GLW::DMSmooth:			renderModeSmoothAct->setChecked(true);  				break;
			case GLW::DMFlatWire:		renderModeFlatLinesAct->setChecked(true);				break;
			case GLW::DMHidden:			renderModeHiddenLinesAct->setChecked(true);			break;
		}
    colorModePerFaceAct->setEnabled(HasPerFaceColor(GLA()->mm->cm));
		switch (rm.colorMode)
		{
			case GLW::CMNone:			colorModeNoneAct->setChecked(true);	      break;
			case GLW::CMPerVert:	colorModePerVertexAct->setChecked(true);  break;
			case GLW::CMPerFace:	colorModePerFaceAct->setChecked(true);    break;
		}

		lastFilterAct->setEnabled(false);
		if(GLA()->getLastAppliedFilter() != NULL)
		{
			lastFilterAct->setText(QString("Apply filter ") + GLA()->getLastAppliedFilter()->text());
			lastFilterAct->setEnabled(true);
		}
		else
		{
			lastFilterAct->setText(QString("Apply filter "));
		}


    if(GLA()->getEditAction()) 
    {
      endEditModeAct->setChecked(false);
      GLA()->getEditAction()->setChecked(true);
    }
    else  endEditModeAct->setChecked(true);

		showLogAct->setChecked(GLA()->isLogVisible());
		showInfoPaneAct->setChecked(GLA()->isInfoAreaVisible());
		showTrackBallAct->setChecked(GLA()->isTrackBallVisible());
		backFaceCullAct->setChecked(GLA()->getCurrentRenderMode().backFaceCull);
		renderModeTextureAct->setEnabled(GLA()->mm && !GLA()->mm->cm.textures.empty());
		renderModeTextureAct->setChecked(GLA()->getCurrentRenderMode().textureMode != GLW::TMNone);
		
		setLightAct->setIcon(rm.lighting ? QIcon(":/images/lighton.png") : QIcon(":/images/lightoff.png") );
		setLightAct->setChecked(rm.lighting);

		setFancyLightingAct->setChecked(rm.fancyLighting);
		setDoubleLightingAct->setChecked(rm.doubleSideLighting);
		setSelectionRenderingAct->setChecked(rm.selectedFaces);

		foreach (QAction *a,TotalDecoratorsList){a->setChecked(false);}
		if(GLA()->iDecoratorsList){
			pair<QAction *,MeshDecorateInterface *> p;
			foreach (p,*GLA()->iDecoratorsList){p.first->setChecked(true);}
		}
	}
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
  qDebug("dragEnterEvent: %s",event->format());
  QDrag *drag=new QDrag(this);

}
void MainWindow::dropEvent ( QDropEvent * event )  
{
  qDebug("dropEvent: %s",event->format());
}
void MainWindow::applyLastFilter()
{
  GLA()->getLastAppliedFilter()->activate(QAction::Trigger);
}
void MainWindow::showFilterScript()
{
  FilterScriptDialog dialog(this);
	dialog.setScript(&(GLA()->filterHistory));
	if (dialog.exec()==QDialog::Accepted) 
	{
			runFilterScript();
      return ;
	}

}

void MainWindow::runFilterScript()
{
  FilterScript::iterator ii;
  for(ii= GLA()->filterHistory.actionList.begin();ii!= GLA()->filterHistory.actionList.end();++ii)
  {
    QAction *action = filterMap[ (*ii).first];
	  MeshFilterInterface *iFilter = qobject_cast<MeshFilterInterface *>(action->parent());

    int req=iFilter->getRequirements(action);
    GLA()->mm->updateDataMask(req);
    iFilter->applyFilter( action, *(GLA()->mm), (*ii).second, QCallBack );
    GLA()->log.Log(GLLogStream::Info,"Re-Applied filter %s",qPrintable((*ii).first));
	}
}
// /////////////////////////////////////////////////
// The Very Important Procedure of applying a filter
// /////////////////////////////////////////////////

void MainWindow::applyFilter()
{
	QAction *action = qobject_cast<QAction *>(sender());
	MeshFilterInterface *iFilter = qobject_cast<MeshFilterInterface *>(action->parent());
  // (1) Ask for filter requirements (eg a filter can need topology, border flags etc)
  //    and statisfy them
  int req=iFilter->getRequirements(action);
  GLA()->mm->updateDataMask(req);

  
  // (2) Ask for filter parameters (e.g. user defined threshold that could require a widget)
  FilterParameter par;
  bool ret=iFilter->getParameters(action, GLA(),*(GLA()->mm), par);

  if(!ret) return;
	
  // (3) save the current filter and its parameters in the history
  GLA()->filterHistory.actionList.append(qMakePair(action->text(),par));

  qDebug("Filter History size %i",GLA()->filterHistory.actionList.size());
  qDebug("Filter History Last entry %s",qPrintable (GLA()->filterHistory.actionList.front().first));

  qb->show();
  iFilter->setLog(&(GLA()->log));
  // (4) Apply the Filter 
  GLA()->mm->busy=true;
  ret=iFilter->applyFilter(action, *(GLA()->mm), par, QCallBack);
  GLA()->mm->busy=false;
	
  // (5) Apply post filter actions (e.g. recompute non updated stuff if needed)

	if(ret)
	{
		GLA()->log.Log(GLLogStream::Info,"Applied filter %s",qPrintable(action->text()));
		GLA()->setWindowModified(true);
		GLA()->setLastAppliedFilter(action);
		lastFilterAct->setText(QString("Apply filter ") + action->text());  
		lastFilterAct->setEnabled(true);
	}

  // at the end for filters that change the color set the appropriate color mode
  if(iFilter->getClass(action)==MeshFilterInterface::FaceColoring ) {
    GLA()->setColorMode(vcg::GLW::CMPerFace);
    GLA()->mm->ioMask|=MeshModel::IOM_FACECOLOR;
  }
  if(iFilter->getClass(action)==MeshFilterInterface::VertexColoring ){
    GLA()->setColorMode(vcg::GLW::CMPerVert);
    GLA()->mm->ioMask|=MeshModel::IOM_VERTCOLOR;
    GLA()->mm->ioMask|=MeshModel::IOM_VERTQUALITY;
  }
if(iFilter->getClass(action)==MeshFilterInterface::Selection )
    GLA()->setSelectionRendering(true);

  qb->reset();
  updateMenus();
  
}
void MainWindow::endEditMode()
{
  if(!GLA()) return;
  if(GLA()->getEditAction())
  {
	  GLA()->getEditAction()->setChecked(false);
    GLA()->endEdit();
  }
  else
    if(GLA()->getLastAppliedEdit())
    {	
      QAction *action = qobject_cast<QAction *>(GLA()->getLastAppliedEdit());
	    MeshEditInterface *iEdit = qobject_cast<MeshEditInterface *>(action->parent());
      GLA()->setEdit(iEdit,action);
      iEdit->StartEdit(action,*(GLA()->mm),GLA());
	    GLA()->log.Log(GLLogStream::Info,"Started Mode %s",qPrintable (action->text()));
      GLA()->setSelectionRendering(true);
    }
  updateMenus();
}
void MainWindow::applyEditMode()
{
	QAction *action = qobject_cast<QAction *>(sender());
	MeshEditInterface *iEdit = qobject_cast<MeshEditInterface *>(action->parent());
  GLA()->setEdit(iEdit,action);
  GLA()->setLastAppliedEdit(action);

  iEdit->StartEdit(action,*(GLA()->mm),GLA());
	GLA()->log.Log(GLLogStream::Info,"Started Mode %s",qPrintable (action->text()));
  GLA()->setSelectionRendering(true);
  updateMenus();
}

void MainWindow::applyRenderMode()
{
	QAction *action = qobject_cast<QAction *>(sender());		// find the action which has sent the signal 
	
	// Make the call to the plugin core
	MeshRenderInterface *iRenderTemp = qobject_cast<MeshRenderInterface *>(action->parent());
	iRenderTemp->Init(action,*(GLA()->mm),GLA()->getCurrentRenderMode(),GLA());

	if(action->text() == tr("None"))
	{
		GLA()->log.Log(GLLogStream::Info,"No Shader");
		GLA()->setRenderer(0,0); //vertex and fragment programs not supported
	} else {
		if(iRenderTemp->isSupported())
		{
			GLA()->setRenderer(iRenderTemp,action);
			GLA()->log.Log(GLLogStream::Info,"%s",qPrintable(action->text()));	// Prints out action name
		}
		else
		{
			GLA()->setRenderer(0,0); //vertex and fragment programs not supported
			GLA()->log.Log(GLLogStream::Warning,"Shader not supported!");
		}
	}
}


void MainWindow::applyColorMode()
{
	QAction *action = qobject_cast<QAction *>(sender());
	MeshFilterInterface *iColorTemp = qobject_cast<MeshFilterInterface *>(action->parent());
  iColorTemp->setLog(&(GLA()->log));
  //iColorTemp->Compute(action,*(GLA()->mm ),GLA()->getCurrentRenderMode(), GLA());
  GLA()->log.Log(GLLogStream::Info,"Applied colorize %s",action->text().toLocal8Bit().constData());
  updateMenus();
}

void MainWindow::applyDecorateMode()
{
	QAction *action = qobject_cast<QAction *>(sender());		// find the action which has sent the signal 

	MeshDecorateInterface *iDecorateTemp = qobject_cast<MeshDecorateInterface *>(action->parent());
	if(GLA()->iDecoratorsList==0){
		GLA()->iDecoratorsList= new list<pair<QAction *,MeshDecorateInterface *> >;
		GLA()->iDecoratorsList->push_back(make_pair(action,iDecorateTemp));
		GLA()->log.Log(GLLogStream::Info,"Enable Decorate mode %s",action->text().toLocal8Bit().constData());
	}else{
		bool found=false;
		pair<QAction *,MeshDecorateInterface *> p;
		foreach(p,*GLA()->iDecoratorsList){
			if(iDecorateTemp==p.second && p.first->text()==action->text()){
				GLA()->iDecoratorsList->remove(p);
				GLA()->log.Log(0,"Disabled Decorate mode %s",action->text().toLocal8Bit().constData());
				found=true;
			} 
		}
		if(!found){
			GLA()->iDecoratorsList->push_back(make_pair(action,iDecorateTemp));
			GLA()->log.Log(GLLogStream::Info,"Enable Decorate mode %s",action->text().toLocal8Bit().constData());
		}
	}
}

bool MainWindow::QCallBack(const int pos, const char * str)
{
  MainWindow::globalStatusBar()->showMessage(str,1000);
	qb->setValue(pos);
	
  //if(qb==0) return true;
	//qb->setWindowTitle (str);
	//qApp->processEvents();
	//if (qb->wasCanceled())
	//{
	//	qb->reset();
	//	return false;
	//}
	return true;
}

void MainWindow::setLight()			     
{
	GLA()->setLight(!GLA()->getCurrentRenderMode().lighting);
	updateMenus();
};

void MainWindow::setDoubleLighting()
{
	const RenderMode &rm = GLA()->getCurrentRenderMode();
	GLA()->setLightMode(!rm.doubleSideLighting,LDOUBLE);
}

void MainWindow::setFancyLighting()
{
	const RenderMode &rm = GLA()->getCurrentRenderMode();
	GLA()->setLightMode(!rm.fancyLighting,LFANCY);
}

void MainWindow::setLightingProperties()
{
	// retrieve current lighting settings
	GLLightSetting GLlightsetting = GLA()->getLightSettings();
	
	// customize them
	LightingDialog dlg(GLlightsetting, this);
	if (dlg.exec() == QDialog::Accepted)
	{
		// update light settings
		dlg.lightSettingsToGL(GLlightsetting);
		GLA()->setLightSettings(GLlightsetting);

		// update lighting model
		GLA()->setLightModel();
	}
}

void MainWindow::toggleBackFaceCulling()
{
	RenderMode &rm = GLA()->getCurrentRenderMode();
	GLA()->setBackFaceCulling(!rm.backFaceCull);
}

void MainWindow::toggleSelectionRendering()
{
	RenderMode &rm = GLA()->getCurrentRenderMode();
	GLA()->setSelectionRendering(!rm.selectedFaces);
}




void MainWindow::open(QString fileName)
{
	// Opening files in a transparent form (IO plugins contribution is hidden to user)
	QStringList filters;
	
	// HashTable storing all supported formats togheter with
	// the (1-based) index  of first plugin which is able to open it
	QHash<QString, int> allKnownFormats;
	
	LoadKnownFilters(meshIOPlugins, filters, allKnownFormats,IMPORT);

	if (fileName.isEmpty())
		fileName = QFileDialog::getOpenFileName(this,tr("Open File"),".", filters.join("\n"));
	
	if (fileName.isEmpty())	return;

	QFileInfo fi(fileName);
	// this change of dir is needed for subsequent textures/materials loading
	QDir::setCurrent(fi.absoluteDir().absolutePath());
	
	QString extension = fi.suffix();
	
	// retrieving corresponding IO plugin
	int idx = allKnownFormats[extension.toLower()];
	if (idx == 0)
	{	
		QString errorMsgFormat = "Error encountered while opening file:\n\"%1\"\n\nError details: The \"%2\" file extension does not correspond to any supported format.";
		QMessageBox::critical(this, tr("Opening Error"), errorMsgFormat.arg(fileName, extension));
		return;
	}
	MeshIOInterface* pCurrentIOPlugin = meshIOPlugins[idx-1];
	
	qb->show();
	int mask = 0;
	MeshModel *mm= new MeshModel();	
	if (!pCurrentIOPlugin->open(extension, fileName, *mm ,mask,QCallBack,this /*gla*/))
		delete mm;
	else{
		GLArea *gla;
		gla=new GLArea(workspace);
		gla->mm=mm;
    gla->mm->ioMask = mask;				// store mask into model structure
    
		gla->setFileName(fileName);
		gla->setWindowTitle(QFileInfo(fileName).fileName()+tr("[*]"));
    gla->showInfoArea(true);
		workspace->addWindow(gla);
		if(workspace->isVisible()) gla->showMaximized();
		setCurrentFile(fileName);
		
    if( mask & vcg::tri::io::Mask::IOM_FACECOLOR)
			gla->setColorMode(GLW::CMPerFace);
		if( mask & vcg::tri::io::Mask::IOM_VERTCOLOR)
    {
      gla->mm->storeVertexColor();
			gla->setColorMode(GLW::CMPerVert);
    }
    renderModeTextureAct->setChecked(false);
		renderModeTextureAct->setEnabled(false);
		if(!GLA()->mm->cm.textures.empty())
		{
			renderModeTextureAct->setChecked(true);
			renderModeTextureAct->setEnabled(true);
			GLA()->setTextureMode(GLW::TMPerWedgeMulti);
		}
		vcg::tri::UpdateNormals<CMeshO>::PerVertexNormalizedPerFace(mm->cm);																																			 
    updateMenus();
    vcg::tri::Clean<CMeshO>::RemoveDegenerateFace(mm->cm);
    GLA()->mm->busy=false;
	}

 

	qb->reset();
}

void MainWindow::openRecentFile()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action)	open(action->data().toString());
}

void MainWindow::reload()
{
	// Discards changes and reloads current file 
	// save current file name
	QString file = GLA()->getFileName();

	// close current window
	workspace->closeActiveWindow();

	// open a new window with old file
	open(file);
}


bool MainWindow::saveAs()
{	
	QStringList filters;
	
	QHash<QString, int> allKnownFormats;
	
	LoadKnownFilters(meshIOPlugins, filters, allKnownFormats,EXPORT);

	QString fileName;

	if (fileName.isEmpty())
		fileName = QFileDialog::getSaveFileName(this,tr("Save File"),".", filters.join("\n"));
	
	bool ret = false;

	QStringList fs = fileName.split(".");
	
	if(!fileName.isEmpty() && fs.size() < 2)
	{
		QMessageBox::warning(new QWidget(),"Save Error","You must specify file extension!!");
		return ret;
	}

	if (!fileName.isEmpty())
	{
		QString extension = fileName;
		extension.remove(0, fileName.lastIndexOf('.')+1);
	
		QStringListIterator itFilter(filters);

		int idx = allKnownFormats[extension.toLower()];
		if (idx == 0)
		{
			QMessageBox::warning(this, "Unknown type", "File extension not supported!");
			return false;
		}
		MeshIOInterface* pCurrentIOPlugin = meshIOPlugins[idx-1];
		
		int capability = pCurrentIOPlugin->GetExportMaskCapability(extension);
		
		int mask = vcg::tri::io::SaveMaskToExporter::GetMaskToExporter(this->GLA()->mm, capability);
		if(mask == -1) 
			return false;
		qb->show();
		ret = pCurrentIOPlugin->save(extension, fileName, *this->GLA()->mm ,mask,QCallBack,this);
		qb->reset();
	}	
  GLA()->setWindowModified(false);
	return ret;
}

bool MainWindow::saveSnapshot()
{

	SaveSnapshotDialog dialog(this);
	
	SnapshotSetting ss = GLA()->getSnapshotSetting();
	dialog.setValues(ss);

	if (dialog.exec()==QDialog::Accepted) 
	{
		ss=dialog.getValues();
		GLA()->setSnapshotSetting(ss);
		GLA()->saveSnapshot();
		return true;
	}

	return false;
}
void MainWindow::about()
{
	QDialog *about_dialog = new QDialog();
	Ui::aboutDialog temp;
	temp.setupUi(about_dialog);
	temp.labelMLName->setText(appName()+"   ("+__DATE__+")");
	//about_dialog->setFixedSize(566,580);
	about_dialog->show();
}

void MainWindow::aboutPlugins()
{
	PluginDialog dialog(pluginsDir.path(), pluginFileNames, this);
	dialog.exec();
}
void MainWindow::showToolbarFile(){
		mainToolBar->setVisible(!mainToolBar->isVisible());
}

void MainWindow::showToolbarRender(){
	renderToolBar->setVisible(!renderToolBar->isVisible());
}

void MainWindow::showLog()			 {if(GLA() != 0)	GLA()->showLog(!GLA()->isLogVisible());}
void MainWindow::showInfoPane()  {if(GLA() != 0)	GLA()->showInfoArea(!GLA()->isInfoAreaVisible());}
void MainWindow::showTrackBall() {if(GLA() != 0) 	GLA()->showTrackBall(!GLA()->isTrackBallVisible());}
void MainWindow::resetTrackBall(){if(GLA() != 0)	GLA()->resetTrackBall();}
void MainWindow::setCustomize()
{
	CustomDialog dialog(this);
	ColorSetting cs=GLA()->getCustomSetting();
	dialog.loadCurrentSetting(cs.bColorBottom,cs.bColorTop,cs.lColor,GLA()->getLogLevel());
	if (dialog.exec()==QDialog::Accepted) 
	{
		// If press Ok set the selected colors in glArea
		cs.bColorBottom=dialog.getBkgBottomColor();
		cs.bColorTop=dialog.getBkgTopColor();
		cs.lColor=dialog.getLogAreaColor();
    GLA()->setCustomSetting(cs);
		GLA()->setLogLevel(dialog.getLogLevel());
	}	
}

void MainWindow::renderBbox()        { GLA()->setDrawMode(GLW::DMBox     ); }
void MainWindow::renderPoint()       { GLA()->setDrawMode(GLW::DMPoints  ); }
void MainWindow::renderWire()        { GLA()->setDrawMode(GLW::DMWire    ); }
void MainWindow::renderFlat()        { GLA()->setDrawMode(GLW::DMFlat    ); }
void MainWindow::renderFlatLine()    { GLA()->setDrawMode(GLW::DMFlatWire); }
void MainWindow::renderHiddenLines() { GLA()->setDrawMode(GLW::DMHidden  ); }
void MainWindow::renderSmooth()      { GLA()->setDrawMode(GLW::DMSmooth  ); }
void MainWindow::renderTexture()
{
	QAction *a = qobject_cast<QAction* >(sender());
	GLA()->setTextureMode(!a->isChecked() ? GLW::TMNone : GLW::TMPerWedgeMulti);	
}


void MainWindow::fullScreen(){
  if(!isFullScreen())
  {
	  toolbarState = saveState();
	  menuBar()->hide();
	  mainToolBar->hide();
	  renderToolBar->hide();
    globalStatusBar()->hide();
	  setWindowState(windowState()^Qt::WindowFullScreen);
	  bool found=true;
	  //Caso di piu' finestre aperte in tile:
	  if((workspace->windowList()).size()>1){
		  foreach(QWidget *w,workspace->windowList()){if(w->isMaximized()) found=false;}
		  if (found)workspace->tile();
	  }
  }
  else
  {
    menuBar()->show();
		restoreState(toolbarState);
    globalStatusBar()->show();

		setWindowState(windowState()^ Qt::WindowFullScreen);
		bool found=true;
		//Caso di piu' finestre aperte in tile:
		if((workspace->windowList()).size()>1){
			foreach(QWidget *w,workspace->windowList()){if(w->isMaximized()) found=false;}
			if (found){workspace->tile();}
		}
		fullScreenAct->setChecked(false);
  }
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
  if(e->key()==Qt::Key_Return && e->modifiers()==Qt::AltModifier)
  {
    fullScreen();
    e->accept();
  }
  else e->ignore();
}