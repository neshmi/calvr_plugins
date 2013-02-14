#include "ArtifactVis2.h"
#ifdef WITH_OSSIMPLANET
#include "../OssimPlanet/OssimPlanet.h"
#endif

#include <iostream>
#include <sstream>
#include <iostream>
#include <fstream>

#include <cmath>
#include <algorithm>
#include <vector>
#include <sys/stat.h>
#include <time.h>
#include <cstdlib>

#ifndef WIN32
#include <unistd.h>
#else
#include <direct.h>
#endif

#include <cvrConfig/ConfigManager.h>
#include <cvrKernel/PluginHelper.h>
#include <cvrKernel/PluginManager.h>
#include <cvrKernel/SceneManager.h>
#include <cvrKernel/SceneObject.h>
//#include <cvrKernel/InteractionManager.h>
#include <cvrMenu/MenuSystem.h>
//#include <cvrUtil/LocalToWorldVisitor.h>
#include <cvrUtil/TextureVisitors.h>
#include <PluginMessageType.h>
#include <cvrKernel/ComController.h>
#include <cvrKernel/CVRViewer.h>
//#include "DesignStateIntersector.h"
//#include "DesignObjectIntersector.h"
//#include <cvrKernel/SceneManager.h>

#include <osg/Vec4>

#include <osgUtil/SceneView>
#include <osg/Camera>


#include <osg/PointSprite>
#include <osg/BlendFunc>
#include <osg/StateAttribute>
#include <osg/Point>
#include <osg/TexEnv>
#include <osg/GLExtensions>

#include <osg/CullFace>
#include <osg/Matrix>
#include <osg/ShapeDrawable>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osg/PositionAttitudeTransform>
#include <osg/PolygonMode>
//#include <osg/ImageUtils>
//#include <osg/Image>
//#include </home/calvr/calvr_plugins/calit2/SpaceNavigator/SpaceNavigator.h>


#include <mxml.h>

//#include <osgDB/WriteFile>

const double m2mm = 1.0; //Now working in Meters, affects radius of cylinder and boxes
const double rec2mm = m2mm / 1000.0;

using namespace osg;
using namespace std;
using namespace cvr;

CVRPLUGIN(ArtifactVis2)
ArtifactVis2* ArtifactVis2::_artifactvis2 = NULL;
ArtifactVis2::ArtifactVis2()
{
    _ossim = false;
}
ArtifactVis2::~ArtifactVis2()
{
}
/*
* Returns an instance for use with other programs.
*/

ArtifactVis2* ArtifactVis2::instance()
{
    if (!_artifactvis2)
    {
        _artifactvis2 = new ArtifactVis2();
    }

    return _artifactvis2;
}

void ArtifactVis2::message(int type, char* data)
{
    /*
    if(type == OE_TRANSFORM_POINTER)
    {
    OsgEarthRequest * request = (OsgEarthRequest*) data;

    }
    */
    _osgearth = true;
}

bool ArtifactVis2::init()
{
    lineGroupsEditing = false;
    _currentScroll = 0;
    modelDropped = false; 
    ArtifactVis2On = false;
     newFileAvailable = false;
    secondInitComplete = false;
    pointGeode = new osg::Geode();
    _modelFileNode = NULL;
    std::cerr << "ArtifactVis2 init\n";
    _root = new osg::MatrixTransform();
    _tablesMenu = NULL;
    _shiftActive = false;
    _grabActive = false;
    _rotActive = false;

    _defaultMaterial = new Material();
    _defaultMaterial->setColorMode(Material::AMBIENT_AND_DIFFUSE);
    _defaultMaterial->setDiffuse(Material::FRONT, osg::Vec4(1.0, 1.0, 1.0, 1.0));
    //Create Basic Menu

    _avMenu = new SubMenu("ArtifactVis2", "ArtifactVis2");
    _avMenu->setCallback(this);
    _turnOnArtifactVis2 = new MenuCheckbox("Turn On", ConfigManager::getBool("Plugin.ArtifactVis2.StartArtifactVis2"));
    _turnOnArtifactVis2->setCallback(this);
    _avMenu->addItem(_turnOnArtifactVis2);
    MenuSystem::instance()->addMenuItem(_avMenu);

    SceneManager::instance()->getObjectsRoot()->addChild(_root);
    _sphereRadius = 0.03;
    _vertexRadius = 0.01;
    _activeArtifact  = -1;


    std::cerr << "ArtifactVis2 init done.\n";

    if(ConfigManager::getBool("Plugin.ArtifactVis2.StartArtifactVis2"))
    {
      secondInit();
    }
    return true;
}


/*
 Loads in all existing models of the form 3dModelFolder/DC/DC.obj, where DC is the two letter DCode.
 Has space for ALL possible DC codes (26^2).
*/
void ArtifactVis2::loadModels()
{
     string dc = "ZZ"; 
     string modelPathDefault = ConfigManager::getEntry("Plugin.ArtifactVis2.3DModelFolder").append("dcode_models/" + dc + "/" + dc + ".obj");
    defaultDcModel = osgDB::readNodeFile(modelPathDefault);
    for (int i = 0; i < 26; i++)
    {
        for (int j = 0 ; j < 26; j++)
        {
            char c1 = i + 65;
            char c2 = j + 65;
            stringstream ss;
            ss << c1 << c2;
            string dc = ss.str();
            string modelPath = ConfigManager::getEntry("Plugin.ArtifactVis2.3DModelFolder").append("dcode_models/" + dc + "/" + dc + ".obj");

            if (modelExists(modelPath.c_str()))
            {
                //cout << "Model " << modelPath << " Exists \n";
                _models[i * 26 + j] = osgDB::readNodeFile(modelPath);
                _modelLoaded[i * 26 + j] = _models[i * 26 + j].valid();
                
            }
            else
            {
                //cout << "Model " << modelPath << " Not Exists \n";
                _models[i * 26 + j] = NULL;
                _modelLoaded[i * 26 + j] = false;
               // _models[i * 26 + j] = defaultDcModel;
               // _modelLoaded[i * 26 + j] = _models[i * 26 + j].valid();
            }
        }
    }

}
bool ArtifactVis2::processEvent(InteractionEvent* event)
{
    TrackedButtonInteractionEvent* tie = event->asTrackedButtonEvent();
    KeyboardInteractionEvent* keyTie = event->asKeyboardEvent();
if(ArtifactVis2On)
{
if(event->asKeyboardEvent() && ArtifactVis2On)
{
/*
	bool keyPressed = true;
	if(keyTie->getMod()==2 && keyTie->getKey() == 65506)
	{	
	_shiftActive = true;	
	cerr << "Key: " << keyTie->getKey()  << " " << keyTie->getMod() <<" Pressed\n";
	}
	else if(keyTie->getKey() == 65506)
	{
	_shiftActive = false;
	cerr << "Key: " << keyTie->getKey()  << " " << keyTie->getMod() <<" UnPressed\n";
	}
*/  
        if(keyTie->getKey() != currentKey)
          {
          int inc = findActiveAnnot();
          if(inc != -1)
          {
	  //cerr << "Key: " << keyTie->getKey()  <<" \n";
          int code = keyTie->getKey();
          string character = getCharacterAscii(code);
          
          string oldText = _annotations[inc]->textNode->getText().createUTF8EncodedString();
          //cerr << "Old Text: " << oldText << " " << keyTie->getKey() << "\n";
          string newText;
          if(code == 65288)
          {
            oldText.erase(oldText.length()-1,1);
            newText = oldText;
          }
          else
          {
          newText = oldText.append(character);
          }
          _annotations[inc]->textNode->setText(newText);
          currentKey = keyTie->getKey();
          }
          else
          {

          currentKey = NULL;
          }
          }
          else
          {
          currentKey = NULL;
          }
          
          }                 
    if (!tie)
    {
        return false;
    }

    if ((event->getInteraction() == BUTTON_DOWN) && tie->getHand() == 0 && tie->getButton() == 0)
    {
        //bang
    }
        if(false)
            {
/*
        osg::Vec3 pointerOrg, pointerPos;
        osg::Matrixd w2o = PluginHelper::getWorldToObjectTransform();

        //pointerOrg = osg::Vec3(0, 0, 0) * TrackingManager::instance()->getHandMat(0) * w2o;
        //pointerOrg = osg::Vec3(0, 0, 0) * TrackingManager::instance()->getHandMat(0) * w2o;
        pointerPos = osg::Vec3(0, 1000, 0) * TrackingManager::instance()->getHandMat(0);
        pointerOrg = osg::Vec3(0, 0, 0) * TrackingManager::instance()->getHandMat(0);

    DSIntersector::DSIntersector* mDSIntersector = new DSIntersector();
    DOIntersector* mDOIntersector = new DOIntersector();
    mDOIntersector = new DOIntersector();
    mDSIntersector->loadRootTargetNode(SceneManager::instance()->getScene(), NULL);
    mDOIntersector->loadRootTargetNode(NULL, NULL);
        if (tie->getHand() == 0 && tie->getButton() == 0)
        {

            if (tie->getInteraction() == BUTTON_DOWN || tie->getInteraction() == BUTTON_DRAG)
            {
          if(_shiftActive && _grabActive)
	  {
             Vec3 rotVector;
             if(!_rotActive)
             {
               _grabCurrentRot = pointerPos;
               _rotActive = true;
               //rhit = mDSIntersector->getWorldHitPosition();
               cerr << "Rot Started\n";
	     }
             else
             {
		rotVector = _grabCurrentRot - pointerPos;
                rotVector.y() = 0;
                rotVector.z() /= 50;
                rotVector.x() /= 50;
               _grabCurrentRot = pointerPos;
		//For hand orientation info
		osg::Quat handRot = TrackingManager::instance()->getHandMat(0).getRotate();
  		//  cerr << "rx: " << handRot.x() << " ry : " << handRot.y() << " rz : " << handRot.z() << " rw : " << handRot.w()  << "\n"; 
    		Matrix tmat;
    		Vec3 center = _hudRoot[0]->getBound().center();
    		center = rhit;
    		if(rhit.x() < 0)
    		{
			rhit.x() *= -1;
    		}
    		if(rhit.z() < 0)
    		{
			rhit.z() *= -1;
    		}
    		cerr << "x: " << center.x() << " y: " << center.y() << " z: " << center.z() << "\n"; 
    		tmat.makeTranslate(Vec3(0,0,0));
    		double rx = rotVector.z();
    		double ry = rotVector.y();
    		double rz = rotVector.x();
   		// cerr << "rx: " << rx << " ry: " << ry << " rz: " << rz << "\n";
        	Vec3 xa = Vec3(1.0, 0.0, 0.0);
        	Vec3 ya = Vec3(0.0, 1.0, 0.0);
        	Vec3 za = Vec3(0.0, 0.0, 1.0);
        	Matrix rot;
        	rot.makeRotate(rx, xa, ry, ya, rz, za);
        	Matrixd gotoMat = osg::Matrix::translate(-center)  * rot * osg::Matrix::translate(center);
                osg::Matrix inverseMatrix = osg::Matrix::inverse(gotoMat);

		  _hudRoot[0]->setMatrix(inverseMatrix * _hudRoot[0]->getMatrix());
                  Matrix ctrans = _hudRoot[0]->getMatrix();
                  Vec3 trans = ctrans.getTrans();
                 // cerr << "x: " << trans.x() << " y: " << trans.y() << " z: " << trans.z() << "\n"; 

             }
	  }
          else
	  {
            if (mDSIntersector->test(pointerOrg, pointerPos))
            {
	      if(!_shiftActive)
	      {
                 updateSceneObjectMovement(mDSIntersector, tie);
	     }
             else
            {
		  _grabActive = true;
        //mWireframeGeode->setScaleVect(snapPos - mWireframeGeode->getInitPosition());
	     }
	    }
            }
           
} 
            else if (tie->getInteraction() == BUTTON_UP)
            {
                //bool res = mCAVEDesigner->inputDevReleaseEvent();
                //return res;
                cerr << "-";
                _hudActive = false;
		_grabActive = false;
                _rotActive = false;
            }
	}
  */   
/*
		int i = 0;
		if(i == 0)
		{
		cerr << "Started:\n";
                    std::map<int,osg::Vec3> _currentPoint;
                    osg::Vec3 ray;
                    ray = _currentPoint[tie->getHand()]
                            - tie->getTransform().getTrans();

                    float _moveDistance;
                    osg::Vec3 _menuPoint;
                    _moveDistance = ray.length();
                   / _menuP/int = _currentPoint[tie->getHand()]
                            * osg::Matrix::inverse(_hudRoot[i]->getMatrix());
                   
		     updateHudMovement(i,tie,_moveDistance,_menuPoint);
                
                //return true;
                }
            
*/
            //return false;
   }

    if ((event->getInteraction() == BUTTON_DOWN || event->getInteraction() == BUTTON_DOUBLE_CLICK) && tie->getHand() == 0 && tie->getButton() == 0)
    {

        if (newFileAvailable)
        {
            if(newSelectedFile != "" && newSelectedName != "")
            {
               //int index = _models3d.size(); 

      		if (!modelExists(newSelectedFile.c_str()))
      		{
        		std::cerr << "Unable to open file: " << newSelectedFile << std::endl;
        		//return;
      		}
                else
                {
                bool useHandPos = true;
			if(newSelectedType == "xyb")
			{
                          cerr << "parsing Ply\n";
			  parsePCXml(useHandPos);

			}
			else
			{
			  parseModelXml(useHandPos);
			}
                }
            }
	 
             
            newFileAvailable = false;
          }


    }
    if ((event->getInteraction() == BUTTON_DOWN || event->getInteraction() == BUTTON_DOUBLE_CLICK) && tie->getHand() == 0 && tie->getButton() == 0)
    {

        if (_createAnnotations->getValue())
        {
	   _createAnnotations->setValue(false);
           createAnnotationFile(tie->getTransform());

        }


    }
    if ((event->getInteraction() == BUTTON_DOUBLE_CLICK) && tie->getHand() == 0 && tie->getButton() == 0)
    {
        //Turn Off Editing with Right Click
        if(lineGroupsEditing)
        {
        lineGroupsEditing = false;
	  if(_lineGroups.size() > 0)
	  {
	    for (int i = 0; i < _lineGroups.size(); i++)
	    {
	    if(_lineGroups[i]->editing)
	    {
	      _lineGroups[i]->editing = false;
               closeLineVertex(i);
	    }
            }

          }

	}


    }
    if ((event->getInteraction() == BUTTON_DOWN) && tie->getHand() == 0 && tie->getButton() == 0)
    {
        if(lineGroupsEditing)
        {
	  if(_lineGroups.size() > 0)
	  {
	    for (int i = 0; i < _lineGroups.size(); i++)
	    {
	    if(_lineGroups[i]->editing)
	    {
              lineGroupsEditing = false;   
	      _lineGroups[i]->editing = false;
              addLineVertex(i);
              break;
	    }
            }

          }

	}


    }
    if ((event->getInteraction() == BUTTON_DOWN) && tie->getHand() == 0 && tie->getButton() == 0)
    {

        if (_createMarkup->getValue())
        {
	   _createMarkup->setValue(false);
           startLineObject();
           
        }


    }

    if ((event->getInteraction() == BUTTON_DOWN || event->getInteraction() == BUTTON_DOUBLE_CLICK) && tie->getHand() == 0 && tie->getButton() == 0)
    {
        //Artifact Selection
            cerr << "Select On\n";
        if (_selectArtifactCB->getValue())
        {
            cerr << "Select On\n";
            if (true)
            {
                osg::Matrix w2l = PluginHelper::getWorldToObjectTransform();
                osg::Vec3 start(0, 0, 0);
                osg::Vec3 end(0, 1000000, 0);
                start = start * tie->getTransform() * w2l;
                cerr << "Hand=" << start.x() << " " << start.z() << "\n";
                end = end * tie->getTransform() * w2l;
                int index = -1;
                int queryIndex = -1;
                double distance;
                cerr << "got Interaction\n";

                for (int q = 0; q < _query.size(); q++)
                {
                   // int n = _querySfIndex[q];
                    vector<Artifact*> artifacts = _query[q]->artifacts;
                    if(_query[q]->active) cerr << "Query Active\n";
                    if (_query[q]->active)
                    {
                        for (int i = 0; i < artifacts.size(); i++)
                        {
                            osg::Vec3 num = (artifacts[i]->modelPos - start) ^ (artifacts[i]->modelPos - end);
                            osg::Vec3 denom = end - start;
                            double point2line = num.length() / denom.length();

                            if (point2line <= _sphereRadius)
                            {
                                double point2start = (artifacts[i]->modelPos - start).length2();

                                if (index == -1 || point2start < distance)
                                {
                                    distance = point2start;
                                    index = i;
                                    queryIndex = q;
                                }
                            }
                        }
                    }
                }

                if (index != -1)
                {
                    std::cerr << "Got sphere intersection with index " << index << std::endl;
                    setActiveArtifact(100, CYLINDER, index, queryIndex);
                    return true;
                }
            }
        }
        //Box selection
        else if (_selectCB->getValue() && tie->getInteraction() == BUTTON_DOUBLE_CLICK)
        {
            osg::Matrix w2l = PluginHelper::getWorldToObjectTransform();

            if (!_selectActive)
            {
                _selectStart = osg::Vec3(0, 1000, 0);
                _selectStart = _selectStart * tie->getTransform() * w2l;
                _selectActive = true;
            }
            else
            {
                _selectCurrent = osg::Vec3(0, 1000, 0);
                _selectCurrent = _selectCurrent * tie->getTransform() * w2l;
                _selectActive = false;
            }

            return true;
        }
    }
}
    return false;
}

//Gets the string of the query that would be sent to PGSQL via ArchInterface.
//Includes only the current 'OR' statement, not previous ones. Those are stored in the current_query variable for Tables.
std::string ArtifactVis2::getCurrentQuery(Table* t)
{
    std::stringstream ss;
    std::vector<cvr::SubMenu*>::iterator menu = t->querySubMenu.begin();
    int index = 0;
    bool conditionSelected = false;
    ss << "(";

    for (; menu < t->querySubMenu.end(); menu++)
    {
        if (!t->queryOptions[index]->firstOn().empty())
        {
            if (conditionSelected) ss << " AND ";

            ss << (*menu)->getName() << "=\'" << t->queryOptions[index]->firstOn() << "\'";
            conditionSelected = true;
        }

        index++;
    }

    for (int i = 0; i < t->querySlider.size(); i++)
    {
        if (t->querySlider[i]->getValue())
        {
            if (conditionSelected) ss << " AND ";

            ss <<  t->querySubMenuSlider[i]->getName() << "=\'" << t->queryOptionsSlider[i]->getValue() << "\'";
            conditionSelected = true;
        }
    }

    ss << ")";

    if (!conditionSelected)
    {
        return "";
    }

    return ss.str();
}

void ArtifactVis2::menuCallback(MenuItem* menuItem)
{
    if (menuItem == _turnOnArtifactVis2)
    {
       if(_turnOnArtifactVis2->getValue())
       {
          if(!secondInitComplete)
          {
		secondInit();
   
                ArtifactVis2On = true;
          }
          else
          {
	    if(ConfigManager::getBool("Plugin.ArtifactVis2.MoveCamera"))
	    {

	    int flyIndex = ConfigManager::getInt("Plugin.ArtifactVis2.FlyToDefault");
	    flyTo(flyIndex);
	    }

             _infoPanel->setVisible(true);
             ArtifactVis2On = true;

          }
       }
       else
       {
           //TODO:Turns Off PreFrame and entire Plugin
             _infoPanel->setVisible(false);
             ArtifactVis2On = false;
             turnOffAll();
       }
    }
    if (menuItem == _artifactsDropDown)
    {
       if(!artifactsDropped)
       {
        
         artifactsDropped = true;
         updateDropDowns();
       }
       else
       {

         artifactsDropped = false;
         updateDropDowns();
       }
       return;
    }
    if (menuItem == _lociDropDown)
    {
       if(!lociDropped)
       {
        
         lociDropped = true;
         updateDropDowns();
       }
       else
       {

         lociDropped = false;
         updateDropDowns();
       }
    }
    if (menuItem == _modelDropDown)
    {
       if(!modelDropped)
       {
        
         modelDropped = true;
         updateDropDowns();
       }
       else
       {

         modelDropped = false;
         updateDropDowns();
       }
    }
    if (menuItem == _pcDropDown)
    {
       if(!pcDropped)
       {
        
         pcDropped = true;
         updateDropDowns();
       }
       else
       {

         pcDropped = false;
         updateDropDowns();
       }
    }
    if (menuItem == _bookmarksMenu)
    {
            if (_bookmarksMenu->getValue())
            {
                _bookmarkPanel->setVisible(true);
	    }
            else
            {
                _bookmarkPanel->setVisible(false);
	    }

    }
    if (menuItem == _utilsMenu)
    {
            if (_utilsMenu->getValue())
            {
                _utilsPanel->setVisible(true);
	    }
            else
            {
                _utilsPanel->setVisible(false);
	    }

    }
    if (menuItem == _fileMenu)
    {
            if (_fileMenu->getValue())
            {
                _filePanel->setVisible(true);
	    }
            else
            {
                _filePanel->setVisible(false);
	    }

    }
    if (menuItem == _qsMenu)
    {
            if (_qsMenu->getValue())
            {
                _qsPanel->setVisible(true);
	    }
            else
            {
                _qsPanel->setVisible(false);
	    }

    }
    if (menuItem == _resetFileManager)
    {
             string dir = ConfigManager::getEntry("Plugin.ArtifactVis2.3DModelFolder");
             _currentScroll = 0;
             _currentDir = dir;


             updateFileMenu(dir,0);

    }
    if (menuItem == _downFileManager)
    {
           //  string dir = ConfigManager::getEntry("Plugin.ArtifactVis2.ArchInterfaceFolder");
             _currentScroll++;
             string dir = _currentDir;


             updateFileMenu(dir,_currentScroll);


    }
    if (menuItem == _upFileManager)
    {
           //  string dir = ConfigManager::getEntry("Plugin.ArtifactVis2.ArchInterfaceFolder");
             _currentScroll--;
             if(_currentScroll < 0) _currentScroll = 0;
             string dir = _currentDir;


             updateFileMenu(dir,_currentScroll);

    }
    for (int i = 0; i < fileButton.size(); i++)
    {
        int n = i + (_currentScroll * 10);
        if (menuItem == fileButton[i])
        {
           if(entries[n]->filename == "..")
           {
             //Go up one folder
            string dir = entries[n]->path;
            dir.erase(dir.length()-1);
            cout << dir << endl;
            size_t found=dir.find_last_of("/");
            if (found!=string::npos)
	    {
                 int start = int(found);
                 int length = dir.length();
                 length -= start;
                 dir.erase(start);  
                 dir.append("/");
                 cout << dir << endl;               
                 _currentScroll = 0;
                 _currentDir = dir;
                 updateFileMenu(dir,0);
            }
             break;
           } 
           else if(entries[n]->filetype == "folder")
           {
             //Switch to New Folder
             string dir = entries[n]->path;
             //dir.append("/");
             dir.append(entries[n]->filename);
             dir.append("/");
             cout << dir << endl;               
             _currentScroll = 0;
             _currentDir = dir;
             updateFileMenu(dir,0);
             break;
           }
           else
           {
             //Load File

             newSelectedFile = entries[n]->path;
             newSelectedName = entries[n]->filename;
             newSelectedType = entries[n]->filetype;
            // newSelectedFile.append("/");
             newSelectedFile.append(newSelectedName);
             newFileAvailable = true;
             break;
           }



	}
    }
    for (int i = 0; i < _annotations.size(); i++)
    {
        if (menuItem == _annotations[i]->saveMap)
        {
	        std::cerr << "Save." << std::endl;
                Vec3 pos = _annotations[i]->so->getPosition();
                cerr << "x: " << pos.x() << " y: " << pos.y() << " z: " << pos.z() << std::endl;
                saveAnnotationGraph();
	}
        if (menuItem == _annotations[i]->activeMap)
        {
            if (_annotations[i]->activeMap->getValue())
            {
	        std::cerr << "Active." << std::endl;
                 deactivateAllAnno();
                 _annotations[i]->active = true;
                 _annotations[i]->so->setMovable(true);
                 _annotations[i]->activeMap->setValue(true);
            }
            else
            {

                 deactivateAllAnno();
	        std::cerr << "DeActive." << std::endl;
            }
	}
        if (menuItem == _annotations[i]->visibleMap)
        {
            if (_annotations[i]->visibleMap->getValue())
            {
	        std::cerr << "Active." << std::endl;
                 deactivateAllAnno();
                 _annotations[i]->active = true;
                 _annotations[i]->so->setMovable(true);
                 _annotations[i]->activeMap->setValue(true);
                if(!_annotations[i]->visible)
                {
                 _annotations[i]->so->attachToScene();
		}
            }
            else
            {

                 deactivateAllAnno();
                 _annotations[i]->so->detachFromScene();
                 _annotations[i]->visible = false;
                 _annotations[i]->connectorNode->setNodeMask(0);
            }
	}
        if (menuItem == _annotations[i]->deleteMap)
        {
                 deactivateAllAnno();
                 _annotations[i]->so->detachFromScene();
                 _annotations[i]->visible = false;
                 _annotations[i]->deleted = true;
                 _annotations[i]->connectorNode->setNodeMask(0);
                saveAnnotationGraph();

        }
    }
    for (int i = 0; i < _lineGroups.size(); i++)
    {
        if (menuItem == _lineGroups[i]->saveMap)
        {
	        std::cerr << "Save." << std::endl;
                Vec3 pos = _lineGroups[i]->so->getPosition();
                cerr << "x: " << pos.x() << " y: " << pos.y() << " z: " << pos.z() << std::endl;
               // saveAnnotationGraph();
	}
        if (menuItem == _lineGroups[i]->activeMap)
        {
            if (_lineGroups[i]->activeMap->getValue())
            {
	        std::cerr << "Active." << std::endl;
               //  deactivateAllAnno();
                 _lineGroups[i]->active = true;
                 _lineGroups[i]->so->setMovable(true);
                 _lineGroups[i]->activeMap->setValue(true);
            }
            else
            {
                 _lineGroups[i]->active = false;
                 _lineGroups[i]->so->setMovable(false);
                 _lineGroups[i]->activeMap->setValue(false);

	        std::cerr << "DeActive." << std::endl;
            }
	}
        if (menuItem == _lineGroups[i]->visibleMap)
        {
            if (_lineGroups[i]->visibleMap->getValue())
            {
	        std::cerr << "Active." << std::endl;
                // deactivateAllAnno();
                 _lineGroups[i]->active = true;
                 _lineGroups[i]->so->setMovable(true);
                 _lineGroups[i]->activeMap->setValue(true);
                if(!_lineGroups[i]->visible)
                {
                 _lineGroups[i]->so->attachToScene();
		}
            }
            else
            {

                // deactivateAllAnno();
                 _lineGroups[i]->active = false;
                 _lineGroups[i]->so->setMovable(false);
                 _lineGroups[i]->activeMap->setValue(false);
                 _lineGroups[i]->so->detachFromScene();
                 _lineGroups[i]->visible = false;
            }
	}
        if (menuItem == _lineGroups[i]->deleteMap)
        {
                 //deactivateAllAnno();
                 _lineGroups[i]->active = false;
                 _lineGroups[i]->so->setMovable(false);
                 _lineGroups[i]->activeMap->setValue(false);
                 _lineGroups[i]->so->detachFromScene();
                 _lineGroups[i]->visible = false;
                 _lineGroups[i]->deleted = true;
               // saveAnnotationGraph();

        }
    }
    for (int i = 0; i < _pointClouds.size(); i++)
    {
        if (menuItem == _pointClouds[i]->saveMap)
        {
	        std::cerr << "Save." << std::endl;
                 saveModelConfig(_pointClouds[i], false);
	}
        if (menuItem == _pointClouds[i]->saveNewMap)
        {
	        std::cerr << "Save New." << std::endl;
                 saveModelConfig(_pointClouds[i], true);
	}
        else if (menuItem == _pointClouds[i]->resetMap)
        {
	        std::cerr << "Reset." << std::endl;
               
	}
        else if (menuItem == _pointClouds[i]->activeMap)
        {
            if (_pointClouds[i]->activeMap->getValue())
            {
	        std::cerr << "Active." << std::endl;
                 _pointClouds[i]->active = true;
                 _pointClouds[i]->so->setMovable(true);
                 _pointClouds[i]->activeMap->setValue(true);
            }
            else
            {
                 _pointClouds[i]->active = false;
                 _pointClouds[i]->so->setMovable(false);
                 _pointClouds[i]->activeMap->setValue(false);

	        std::cerr << "DeActive." << std::endl;
            }
	}
        else if (menuItem == _pointClouds[i]->visibleMap)
        {
            if (_pointClouds[i]->visibleMap->getValue())
            {
	        std::cerr << "Visible." << std::endl;
                 _pointClouds[i]->active = true;
                if(!_pointClouds[i]->visible)
                {
                 _pointClouds[i]->so->attachToScene();
		}
                 _pointClouds[i]->visibleMap->setValue(true);
            }
            else
            {
                 _pointClouds[i]->active = false;
                 _pointClouds[i]->so->setMovable(false);
                 _pointClouds[i]->activeMap->setValue(false);
                 _pointClouds[i]->so->detachFromScene();
                 _pointClouds[i]->visible = false;
	        std::cerr << "NotVisible." << std::endl;
            }
	}
        else if (menuItem == _pointClouds[i]->rxMap)
        {
	        //std::cerr << "Rotate." << std::endl;
                Quat mSo = _pointClouds[i]->so->getRotation();
                Quat mRot;
                float deg = _pointClouds[i]->rxMap->getValue();
                if(_pointClouds[i]->rxMap->getValue() > 0)
                {
		  mRot = osg::Quat(0.05, osg::Vec3d(1,0,0),0, osg::Vec3d(0,1,0),0, osg::Vec3d(0,0,1)); 
                }
                else
                {
		  mRot = osg::Quat(-0.05, osg::Vec3d(1,0,0),0, osg::Vec3d(0,1,0),0, osg::Vec3d(0,0,1)); 
                }
                mSo *= mRot;
                 _pointClouds[i]->so->setRotation(mSo);
                _pointClouds[i]->rxMap->setValue(0);
	}
        else if (menuItem == _pointClouds[i]->ryMap)
        {
	        //std::cerr << "Rotate." << std::endl;
                Quat mSo = _pointClouds[i]->so->getRotation();
                Quat mRot;
                float deg = _pointClouds[i]->ryMap->getValue();
                if(_pointClouds[i]->ryMap->getValue() > 0)
                {
		  mRot = osg::Quat(0, osg::Vec3d(1,0,0),0.05, osg::Vec3d(0,1,0),0, osg::Vec3d(0,0,1)); 
                }
                else
                {
		  mRot = osg::Quat(0, osg::Vec3d(1,0,0),-0.05, osg::Vec3d(0,1,0),0, osg::Vec3d(0,0,1)); 
                }
                mSo *= mRot;
                 _pointClouds[i]->so->setRotation(mSo);
                _pointClouds[i]->ryMap->setValue(0);
	}
        else if (menuItem == _pointClouds[i]->rzMap)
        {
	       // std::cerr << "Rotate." << std::endl;
                Quat mSo = _pointClouds[i]->so->getRotation();
                Quat mRot;
                float deg = _pointClouds[i]->rzMap->getValue();
                if(_pointClouds[i]->rzMap->getValue() > 0)
                {
		  mRot = osg::Quat(0, osg::Vec3d(1,0,0),0, osg::Vec3d(0,1,0),0.05, osg::Vec3d(0,0,1)); 
                }
                else
                {
		  mRot = osg::Quat(0, osg::Vec3d(1,0,0),0, osg::Vec3d(0,1,0),-0.05, osg::Vec3d(0,0,1)); 
                }
                mSo *= mRot;
                 _pointClouds[i]->so->setRotation(mSo);
                _pointClouds[i]->rzMap->setValue(0);
	}
    }
    for (int i = 0; i < _models3d.size(); i++)
    {
        if (menuItem == _models3d[i]->saveMap)
        {
	        std::cerr << "Save." << std::endl;
                 saveModelConfig(_models3d[i], false);
	}
        if (menuItem == _models3d[i]->saveNewMap)
        {
	        std::cerr << "Save New." << std::endl;
                 saveModelConfig(_models3d[i], true);
	}
        else if (menuItem == _models3d[i]->resetMap)
        {
	        std::cerr << "Reset." << std::endl;
               
	}
        else if (menuItem == _models3d[i]->activeMap)
        {
            if (_models3d[i]->activeMap->getValue())
            {
	        std::cerr << "Active." << std::endl;
                 _models3d[i]->active = true;
                 _models3d[i]->so->setMovable(true);
                 _models3d[i]->activeMap->setValue(true);
            }
            else
            {
                 _models3d[i]->active = false;
                 _models3d[i]->so->setMovable(false);
                 _models3d[i]->activeMap->setValue(false);

	        std::cerr << "DeActive." << std::endl;
            }
	}
        else if (menuItem == _models3d[i]->visibleMap)
        {
            if (_models3d[i]->visibleMap->getValue())
            {
	        std::cerr << "Visible." << std::endl;
                 _models3d[i]->active = true;
                if(!_models3d[i]->visible)
                {
                 _models3d[i]->so->attachToScene();
		}
                 _models3d[i]->visibleMap->setValue(true);
            }
            else
            {
                 _models3d[i]->active = false;
                 _models3d[i]->so->setMovable(false);
                 _models3d[i]->activeMap->setValue(false);
                 _models3d[i]->so->detachFromScene();
                 _models3d[i]->visible = false;
	        std::cerr << "NotVisible." << std::endl;
            }
	}
        else if (menuItem == _models3d[i]->rxMap)
        {
	        //std::cerr << "Rotate." << std::endl;
                Quat mSo = _models3d[i]->so->getRotation();
                Quat mRot;
                float deg = _models3d[i]->rxMap->getValue();
                if(_models3d[i]->rxMap->getValue() > 0)
                {
		  mRot = osg::Quat(0.05, osg::Vec3d(1,0,0),0, osg::Vec3d(0,1,0),0, osg::Vec3d(0,0,1)); 
                }
                else
                {
		  mRot = osg::Quat(-0.05, osg::Vec3d(1,0,0),0, osg::Vec3d(0,1,0),0, osg::Vec3d(0,0,1)); 
                }
                mSo *= mRot;
                 _models3d[i]->so->setRotation(mSo);
                _models3d[i]->rxMap->setValue(0);
	}
        else if (menuItem == _models3d[i]->ryMap)
        {
	        //std::cerr << "Rotate." << std::endl;
                Quat mSo = _models3d[i]->so->getRotation();
                Quat mRot;
                float deg = _models3d[i]->ryMap->getValue();
                if(_models3d[i]->ryMap->getValue() > 0)
                {
		  mRot = osg::Quat(0, osg::Vec3d(1,0,0),0.05, osg::Vec3d(0,1,0),0, osg::Vec3d(0,0,1)); 
                }
                else
                {
		  mRot = osg::Quat(0, osg::Vec3d(1,0,0),-0.05, osg::Vec3d(0,1,0),0, osg::Vec3d(0,0,1)); 
                }
                mSo *= mRot;
                 _models3d[i]->so->setRotation(mSo);
                _models3d[i]->ryMap->setValue(0);
	}
        else if (menuItem == _models3d[i]->rzMap)
        {
	       // std::cerr << "Rotate." << std::endl;
                Quat mSo = _models3d[i]->so->getRotation();
                Quat mRot;
                float deg = _models3d[i]->rzMap->getValue();
                if(_models3d[i]->rzMap->getValue() > 0)
                {
		  mRot = osg::Quat(0, osg::Vec3d(1,0,0),0, osg::Vec3d(0,1,0),0.05, osg::Vec3d(0,0,1)); 
                }
                else
                {
		  mRot = osg::Quat(0, osg::Vec3d(1,0,0),0, osg::Vec3d(0,1,0),-0.05, osg::Vec3d(0,0,1)); 
                }
                mSo *= mRot;
                 _models3d[i]->so->setRotation(mSo);
                _models3d[i]->rzMap->setValue(0);
	}
    }
    for (int i = 0; i < _artifactAnnoTrack.size(); i++)
    {
       int q = _artifactAnnoTrack[i]->q;
       int art = _artifactAnnoTrack[i]->art;
        if (menuItem == _query[q]->artifacts[art]->annotation->saveMap)
        {
	        std::cerr << "Save." << std::endl;
                Vec3 pos = _query[q]->artifacts[art]->annotation->so->getPosition();

               // saveAnnotationGraph();
	}
        if (menuItem == _query[q]->artifacts[art]->annotation->activeMap)
        {
            if (_query[q]->artifacts[art]->annotation->activeMap->getValue())
            {
	        std::cerr << "Active." << std::endl;
                // deactivateAllArtifactAnno();
                 _query[q]->artifacts[art]->annotation->active = true;
                 _query[q]->artifacts[art]->annotation->so->setMovable(true);
                 _query[q]->artifacts[art]->annotation->activeMap->setValue(true);
                 _artifactAnnoTrack[i]->active = true;
                //Vec3 pos = _annotations[i]->so->getPosition();
            }
            else
            {
                 _query[q]->artifacts[art]->annotation->active = false;
                 _query[q]->artifacts[art]->annotation->so->setMovable(false);
                 _query[q]->artifacts[art]->annotation->activeMap->setValue(false);
                 _artifactAnnoTrack[i]->active = false;

                // deactivateAllArtifactAnno();
	        std::cerr << "DeActive." << std::endl;
            }
	}
        if (menuItem == _query[q]->artifacts[art]->annotation->visibleMap)
        {
            if (_query[q]->artifacts[art]->annotation->visibleMap->getValue())
            {
	        std::cerr << "Visible." << std::endl;
                // deactivateAllArtifactAnno();
                 _query[q]->artifacts[art]->annotation->active = true;
                if(!_query[q]->artifacts[art]->annotation->visible)
                {
                 _query[q]->artifacts[art]->annotation->so->attachToScene();
		}
                 _query[q]->artifacts[art]->annotation->visibleMap->setValue(true);
               //  _artifactAnnoTrack[i]->active = true;
            }
            else
            {
                 _query[q]->artifacts[art]->model->pVisibleMap->setValue(false);
                 _query[q]->artifacts[art]->annotation->active = false;
                 _query[q]->artifacts[art]->annotation->so->setMovable(false);
                 _query[q]->artifacts[art]->annotation->activeMap->setValue(false);
                 _artifactAnnoTrack[i]->active = false;
                 _query[q]->artifacts[art]->annotation->so->detachFromScene();
                 _query[q]->artifacts[art]->annotation->connectorNode->setNodeMask(0);
                // _root->removeChild(_query[q]->artifacts[art]->annotation->connectorGeode);
                 _query[q]->artifacts[art]->annotation->visible = false;
	        std::cerr << "NotVisible." << std::endl;
            }
	}
    }
    for (int i = 0; i < _artifactModelTrack.size(); i++)
    {
       int q = _artifactModelTrack[i]->q;
       int art = _artifactModelTrack[i]->art;
        if (menuItem == _query[q]->artifacts[art]->model->saveMap)
        {
	        std::cerr << "Save." << std::endl;
                Vec3 pos = _query[q]->artifacts[art]->model->so->getPosition();

               // saveAnnotationGraph();
	}
        if (menuItem == _query[q]->artifacts[art]->model->resetMap)
        {
	        std::cerr << "Reset." << std::endl;
                resetArtifactModelOrig(q, art);
               
	}
        if (menuItem == _query[q]->artifacts[art]->model->activeMap)
        {
            if (_query[q]->artifacts[art]->model->activeMap->getValue())
            {
	        std::cerr << "Active." << std::endl;
                 deactivateAllArtifactModel();
                 _query[q]->artifacts[art]->model->active = true;
                 _query[q]->artifacts[art]->model->so->setMovable(true);
                 _query[q]->artifacts[art]->model->activeMap->setValue(true);
                 _artifactModelTrack[i]->active = true;
            }
            else
            {

                 deactivateAllArtifactModel();
	        std::cerr << "DeActive." << std::endl;
            }
	}
        if (menuItem == _query[q]->artifacts[art]->model->visibleMap)
        {
            if (_query[q]->artifacts[art]->model->visibleMap->getValue())
            {
                
	        std::cerr << "Visible." << std::endl;
                 _query[q]->artifacts[art]->model->active = true;
                if(!_query[q]->artifacts[art]->model->visible)
                {
                 _query[q]->artifacts[art]->model->so->attachToScene();
		}
                 _query[q]->artifacts[art]->model->visibleMap->setValue(true);
		
            }
            else
            {
                resetArtifactModelOrig(q, art);
		//Turn Off Model
                 _query[q]->artifacts[art]->model->active = false;
                 _query[q]->artifacts[art]->model->so->setMovable(false);
                 _query[q]->artifacts[art]->model->activeMap->setValue(false);
                 _artifactModelTrack[i]->active = false;
                 _query[q]->artifacts[art]->model->so->detachFromScene();
                 _query[q]->artifacts[art]->model->visible = false;
	        std::cerr << "NotVisible." << std::endl;
                 //Turn Off Annotation
                 _query[q]->artifacts[art]->annotation->active = false;
                 _query[q]->artifacts[art]->annotation->so->setMovable(false);
                 _query[q]->artifacts[art]->annotation->connectorNode->setNodeMask(0);
                 _query[q]->artifacts[art]->annotation->activeMap->setValue(false);
                 _query[q]->artifacts[art]->annotation->visibleMap->setValue(false);
                 _artifactAnnoTrack[i]->active = false;
                 _query[q]->artifacts[art]->annotation->so->detachFromScene();
                 _query[q]->artifacts[art]->annotation->visible = false;
	        //Unmask Default Model
                 
                _query[q]->artifacts[art]->patmt->setNodeMask(0xffffffff);
            }
	}
        if (menuItem == _query[q]->artifacts[art]->model->pVisibleMap)
        {
            if (_query[q]->artifacts[art]->model->pVisibleMap->getValue())
            {
	        std::cerr << "Visible." << std::endl;
                if(!_query[q]->artifacts[art]->annotation->visible)
                {
                 _query[q]->artifacts[art]->annotation->so->attachToScene();
		}
                 _query[q]->artifacts[art]->annotation->connectorNode->setNodeMask(0xffffffff);
                 _query[q]->artifacts[art]->model->pVisibleMap->setValue(true);
                 _query[q]->artifacts[art]->annotation->visibleMap->setValue(true);
                 _query[q]->artifacts[art]->annotation->visible = true;
                 _query[q]->artifacts[art]->annotation->activeMap->setValue(true);
                 _query[q]->artifacts[art]->annotation->so->setMovable(true);
                 _query[q]->artifacts[art]->annotation->active = true;
                 _artifactAnnoTrack[i]->active = true;
            }
            else
            {
                 _query[q]->artifacts[art]->annotation->active = false;
                 _query[q]->artifacts[art]->annotation->so->setMovable(false);
                 _query[q]->artifacts[art]->annotation->connectorNode->setNodeMask(0);
                 _query[q]->artifacts[art]->annotation->activeMap->setValue(false);
                 _query[q]->artifacts[art]->annotation->visibleMap->setValue(false);
                 _artifactAnnoTrack[i]->active = false;
                 _query[q]->artifacts[art]->annotation->so->detachFromScene();
                 _query[q]->artifacts[art]->annotation->visible = false;
	        std::cerr << "NotVisible." << std::endl;
            }
	}
        if (menuItem == _query[q]->artifacts[art]->model->dcMap)
        {
            if (_query[q]->artifacts[art]->model->dcMap->getValue())
            {
                 switchModelType("dc",q,art);
	    }
            else
            {
                 switchModelType("dc",q,art);
	    }
        }
        if (menuItem == _query[q]->artifacts[art]->model->scanMap)
        {
            if (_query[q]->artifacts[art]->model->dcMap->getValue())
            {
                 switchModelType("scan",q,art);
	    }
            else
            {
                 switchModelType("dc",q,art);
	    }
        }
        if (menuItem == _query[q]->artifacts[art]->model->cubeMap)
        {
            if (_query[q]->artifacts[art]->model->dcMap->getValue())
            {
                 switchModelType("cube",q,art);
	    }
            else
            {
                 switchModelType("dc",q,art);
	    }
        }
       for (int n = 0; n < _query[q]->artifacts[art]->model->photoMap.size(); n++)
       {
        if (menuItem == _query[q]->artifacts[art]->model->photoMap[n])
        {
            if (_query[q]->artifacts[art]->model->photoMap[n]->getValue())
            {
                 switchModelType("frame",q,art);
	    }
            else
            {
                 switchModelType("dc",q,art);
	    }
        }
       }

    }
/*
    for(std::map<SceneObject*,MenuButton*>::iterator it = _annotations[0]saveMap.begin(); it != _saveMap.end(); it++)
    {
	    if(menuItem == it->second)
	    {
	        std::cerr << "Save." << std::endl;

	        bool nav;
	        nav = it->first->getNavigationOn();
	        it->first->setNavigationOn(false);

	        locInit[it->first->getName()] = std::pair<float, osg::Matrix>(1.0,it->first->getTransform());

	        it->first->setNavigationOn(nav);

	        writeConfigFile(); 

	    }
    }
*/

    for (int i = 0; i < _showModelCB.size(); i++)
    {
        
        if (menuItem == _showModelCB[i])
        {
            if (_showModelCB[i]->getValue())
            {
               //cerr << "Found Model\n" << endl;
               if(!_models3d[i]->loaded)
               {
                //Model* newModel = new Model();
               addNewModel(i);
               }
               else
               {
		_models3d[i]->so->attachToScene();
		_models3d[i]->visible = true;
		_models3d[i]->active = true;
		_models3d[i]->visibleMap->setValue(true);
		_models3d[i]->activeMap->setValue(true);
               }
            }
            else
            {
               if(_models3d[i]->visible)
               {
		_models3d[i]->so->detachFromScene();
		_models3d[i]->visible = false;
		_models3d[i]->active = false;
		_models3d[i]->visibleMap->setValue(false);
		_models3d[i]->activeMap->setValue(false);

               }


            }
        }
        
    }
    for (int i = 0; i < _showPointCloudCB.size(); i++)
    {
        
        if (menuItem == _showPointCloudCB[i])
        {
            if (_showPointCloudCB[i]->getValue())
            {
               //cerr << "Found Model\n" << endl;
               if(!_pointClouds[i]->loaded)
               {
                //Model* newModel = new Model();
               addNewPC(i);
               }
               else
               {
		_pointClouds[i]->so->attachToScene();
		_pointClouds[i]->visible = true;
		_pointClouds[i]->active = true;
		_pointClouds[i]->visibleMap->setValue(true);
		_pointClouds[i]->activeMap->setValue(true);
               }
            }
            else
            {
               if(_pointClouds[i]->visible)
               {
		_pointClouds[i]->so->detachFromScene();
		_pointClouds[i]->visible = false;
		_pointClouds[i]->active = false;
		_pointClouds[i]->visibleMap->setValue(false);
		_pointClouds[i]->activeMap->setValue(false);

               }


            }
        }
        
    }

/*
    for (int i = 0; i < _showPCCB.size(); i++)
    {
        if (menuItem == _showPCCB[i])
        {
            if (_showPCCB[i]->getValue())
            {
                if (_pcRoot[i]->getNumChildren() == 0)
                    readPointCloud(i);

                _root->addChild(_pcRoot[i].get());
            }
            else
            {
                _root->removeChild(_pcRoot[i].get());
            }
        }

        if (menuItem == _reloadPC[i])
        {
            _root->removeChild(_pcRoot[i].get());
            reloadSite(i);
            readPointCloud(i);
            _root->addChild(_pcRoot[i].get());
        }
    }
    for (int i = 0; i < _showHudCB.size(); i++)
    {
        if (menuItem == _showHudCB[i])
        {
            if (_showHudCB[i]->getValue())
            {
                if (_hudRoot[i]->getNumChildren() == 0)
                    readHudFile(i);
                SceneManager::instance()->getScene()->addChild(_hudRoot[i]);
//                    _root->addChild(_hudRoot[i]);
            }
            else
            {
                SceneManager::instance()->getScene()->removeChild(_hudRoot[i]);
             //   _root->removeChild(_hudRoot[i]);
            }
        }

        if (menuItem == _reloadHud[i])
        {
            //SceneManager::instance()->getScene()->removeChild(_hudRoot[i]);
            //_root->removeChild(_hudRoot[i].get());
            //reloadSite(i);
            //readHudFile(i);
            // SceneManager::instance()->getScene()->addChild(_hudRoot[i]);
            //_root->addChild(_hudRoot[i].get());
        }
    }
*/
    std::vector<Table*>::iterator t = _tables.begin();

    for (; t < _tables.end(); t++)
    {
        for (int i = 0; i < (*t)->querySlider.size(); i++)
        {
            if (menuItem == (*t)->querySlider[i])
                (*t)->query_view->setText((*t)->current_query + getCurrentQuery((*t)));
        }

        for (int i = 0; i < (*t)->queryOptionsSlider.size(); i++)
        {
            if (menuItem == (*t)->queryOptionsSlider[i])
                (*t)->query_view->setText((*t)->current_query + getCurrentQuery((*t)));
        }

        for (int i = 0; i < (*t)->queryOptions.size(); i++)
        {
            if (menuItem == (*t)->queryOptions[i])
                (*t)->query_view->setText((*t)->current_query + getCurrentQuery((*t)));
        }

        if (menuItem == (*t)->clearConditions)
        {
            clearConditions((*t));
            (*t)->current_query = "";
        }

        if (menuItem == (*t)->addOR)
        {
            (*t)->current_query.append(getCurrentQuery((*t)));
            (*t)->current_query.append(" OR ");
            clearConditions((*t));
        }

        if (menuItem == (*t)->removeOR)
        {
            (*t)->current_query = (*t)->current_query.substr(0, (*t)->current_query.rfind("("));
        }

        if (menuItem == (*t)->genQuery)
        {
            //_query[0]->sphereRoot->setNodeMask(0);
            //_query[1]->sphereRoot->setNodeMask(0);
            bool status;

            if (ComController::instance()->isMaster())
            {
                std::stringstream ss;
                ss <<  "./ArchInterface -b ";
                ss << "\"";
                ss << (*t)->name;
                ss << "\" ";
                ss << "\"";
                ss << (*t)->current_query;
                ss << getCurrentQuery((*t));
                ss << "\"";
                const char* current_path = getcwd(NULL, 0);
                chdir(ConfigManager::getEntry("Plugin.ArtifactVis2.ArchInterfaceFolder").c_str());
                cout << ss.str().c_str() << endl;
                system(ss.str().c_str());
                chdir(current_path);
                ComController::instance()->sendSlaves(&status, sizeof(bool));
            }
            else
            {
                ComController::instance()->readMaster(&status, sizeof(bool));
            }

            cout << (*t)->name << "\n";

            if ((*t)->name.find("_a", 0) != string::npos)
            {
                cout << "query0 \n";
                cout << _query[0]->kmlPath << "\n";
                readQuery(_query[0]);
                _root->addChild(_query[0]->sphereRoot);
                //_query[1]->sphereRoot->setNodeMask(0xffffffff);
            }
            else
            {
                cout << "query1 \n";
                cout << _query[1]->kmlPath << "\n";
                readQuery(_query[1]);
                _root->addChild(_query[1]->sphereRoot);
                _query[1]->sphereRoot->setNodeMask(0xffffffff);
            }

            if (_queryOption[0]->getValue())
            {
                if ((*t)->name.find("_a", 0) != string::npos)
                {
                    displayArtifacts(_query[0]);
                    _query[0]->updated = false;
                }
            }

            if (_queryOption[1]->getValue())
            {
                if ((*t)->name.find("_a", 0) == string::npos)
                {
                    _query[1]->updated = false;
                }
            }

            //setupQuerySelectMenu();
        }

        if (menuItem == (*t)->saveQuery)
        {
            const char* current_path = getcwd(NULL, 0);
            chdir(ConfigManager::getEntry("Plugin.ArtifactVis2.ArchInterfaceFolder").c_str());

            if ((*t)->name.find("_a", 0) != string::npos)
            {
                bool status;

                if (ComController::instance()->isMaster())
                {
                    system("./ArchInterface -r \"query\"");
                    ComController::instance()->sendSlaves(&status, sizeof(bool));
                }
                else
                {
                    ComController::instance()->readMaster(&status, sizeof(bool));
                }

                setupQuerySelectMenu();
            }
            else
            {
                bool status;

                if (ComController::instance()->isMaster())
                {
                    system("./ArchInterface -r \"querp\"");
                    ComController::instance()->sendSlaves(&status, sizeof(bool));
                }
                else
                {
                    ComController::instance()->readMaster(&status, sizeof(bool));
                }

                setupQuerySelectMenu();
            }

            chdir(current_path);
        }
    }

    for (int i = 0; i < _queryOptionLoci.size(); i++)
    {
        if (menuItem == _queryOptionLoci[i])
        {
            _query[i]->active = _queryOptionLoci[i]->getValue();

            int n = _queryLociIndex[i];
            if (_queryOptionLoci[i]->getValue())
            {
                if (_query[n]->updated)
                {
                    printf("Query Updated2\n");
                    _query[n]->updated = false;
                }

                _query[n]->sphereRoot->setNodeMask(0xffffffff);
            }
            else
            {
                _query[n]->sphereRoot->setNodeMask(0);
            }
        }
    }
    for (int i = 0; i < _queryOption.size(); i++)
    {
        if (menuItem == _queryOption[i])
        {
            //cerr << "Query Index: " << i << "Active\n";
            int n = _querySfIndex[i];
            _query[n]->active = _queryOption[i]->getValue();
            //cerr << "Query Index is: " << n << "Active\n";
            if (_queryOption[i]->getValue())
            {
                if (_query[n]->updated)
                {
                    if (_query[n]->sf)
                        displayArtifacts(_query[n]);
                    //_root->addChild(_query[i]->sphereRoot);
                    printf("Query Updated\n");
                    _query[n]->updated = false;
                }
                cerr << "i: " << n << "\n";
                _query[n]->sphereRoot->setNodeMask(0xffffffff);
            }
            else
            {
                _query[n]->sphereRoot->setNodeMask(0);
            }
        }

        if (menuItem == _eraseQuery[i])
        {
            bool status;

            if (ComController::instance()->isMaster())
            {
                const char* current_path = getcwd(NULL, 0);
                chdir(ConfigManager::getEntry("Plugin.ArtifactVis2.ArchInterfaceFolder").c_str());
                stringstream ss;
                ss << "./ArchInterface -n \"" << _query[i]->name << "\"";
                cout << ss.str() << endl;
                system(ss.str().c_str());
                chdir(current_path);
                ComController::instance()->sendSlaves(&status, sizeof(bool));
            }
            else
            {
                ComController::instance()->readMaster(&status, sizeof(bool));
            }

            _root->removeChild(_query[i]->sphereRoot);
            _query.erase(_query.begin() + i);
            setupQuerySelectMenu();
        }

        if (menuItem == _centerQuery[i])
        {
            /*
                Matrixd mat;
                mat.makeTranslate(_query[i]->center*-1);
                cout << _query[i]->center.x() << ", " << _query[i]->center.y() << ", " << _query[i]->center.z() << endl;
                SceneManager::instance()->setObjectMatrix(mat);
            */
        }

        if (menuItem == _toggleLabel[i])
        {
            if (_toggleLabel[i]->getValue())
            {
                //cout << "on\n";
                //cout << _query[i]->artifacts.size() << "\n";
                for (int j = 0; j < _query[i]->artifacts.size(); j++)
                    _query[i]->artifacts[j]->showLabel = true;
            }
            else
            {
                //cout << "off\n";
                //cout << _query[i]->artifacts.size() << "\n";
                for (int j = 0; j < _query[i]->artifacts.size(); j++)
                    _query[i]->artifacts[j]->showLabel = false;
            }
        }
    }

    if (menuItem == _locusDisplayMode)
    {
        cout << "LocusDisplayMode=" << _locusDisplayMode->firstOn() << "\n";

        if (_locusDisplayMode->firstOn() == "Wireframe")
        {
            for (int i = 0; i < _query.size(); i++)
            {
                if (!_query[i]->sf)
                {
                    for (int j = 0; j < _query[i]->loci.size(); j++)
                    {
                        _query[i]->sphereRoot->removeChild(_query[i]->loci[j]->top_geode.get());
                        _query[i]->sphereRoot->removeChild(_query[i]->loci[j]->fill_geode.get());
                        _query[i]->sphereRoot->removeChild(_query[i]->loci[j]->line_geode.get());
                        _query[i]->sphereRoot->addChild(_query[i]->loci[j]->fill_geode.get());
                        _query[i]->sphereRoot->addChild(_query[i]->loci[j]->line_geode.get());
                        StateSet* state = _query[i]->loci[j]->fill_geode->getOrCreateStateSet();
                        Material* mat = dynamic_cast<Material*>(state->getAttribute(StateAttribute::MATERIAL, 0));
                        mat->setAlpha(Material::FRONT_AND_BACK, 0.01);
                    }
                }
            }
        }
        else if (_locusDisplayMode->firstOn() == "Solid")
        {
            for (int i = 0; i < _query.size(); i++)
            {
                if (!_query[i]->sf)
                {
                    for (int j = 0; j < _query[i]->loci.size(); j++)
                    {
                        _query[i]->sphereRoot->removeChild(_query[i]->loci[j]->top_geode.get());
                        _query[i]->sphereRoot->removeChild(_query[i]->loci[j]->fill_geode.get());
                        _query[i]->sphereRoot->removeChild(_query[i]->loci[j]->line_geode.get());
                        _query[i]->sphereRoot->addChild(_query[i]->loci[j]->fill_geode.get());
                        _query[i]->sphereRoot->addChild(_query[i]->loci[j]->line_geode.get());
                        StateSet* state = _query[i]->loci[j]->fill_geode->getOrCreateStateSet();
                        Material* mat = dynamic_cast<Material*>(state->getAttribute(StateAttribute::MATERIAL, 0));
                        mat->setAlpha(Material::FRONT_AND_BACK, 0.99);
                    }
                }
            }
        }
        else if (_locusDisplayMode->firstOn() == "Top")
        {
            for (int i = 0; i < _query.size(); i++)
            {
                if (!_query[i]->sf)
                {
                    cout << _query[i]->loci.size() << " size\n";

                    for (int j = 0; j < _query[i]->loci.size(); j++)
                    {
                        //cout << j << "\n";
                        //_query[i]->sphereRoot->removeChild(_query[i]->loci[j]->top_geode);
                        _query[i]->sphereRoot->removeChild(_query[i]->loci[j]->fill_geode.get());
                        _query[i]->sphereRoot->removeChild(_query[i]->loci[j]->line_geode.get());
                        //osg::Geode * test=_query[i]->loci[j]->top_geode;
                        _query[i]->sphereRoot->addChild(_query[i]->loci[j]->top_geode.get());
                        //StateSet * state = _query[i]->loci[j]->top_geode->getOrCreateStateSet();
                        //Material * mat = dynamic_cast<Material*>(state->getAttribute(StateAttribute::MATERIAL,0));
                        //mat->setAlpha(Material::FRONT_AND_BACK,0.4);
                    }
                }
            }
        }
        else
        {
            for (int i = 0; i < _query.size(); i++)
            {
                if (!_query[i]->sf)
                {
                    for (int j = 0; j < _query[i]->loci.size(); j++)
                    {
                        _query[i]->sphereRoot->removeChild(_query[i]->loci[j]->top_geode.get());
                        _query[i]->sphereRoot->removeChild(_query[i]->loci[j]->fill_geode.get());
                        _query[i]->sphereRoot->removeChild(_query[i]->loci[j]->line_geode.get());
                        _query[i]->sphereRoot->addChild(_query[i]->loci[j]->fill_geode.get());
                        _query[i]->sphereRoot->addChild(_query[i]->loci[j]->line_geode.get());
                        StateSet* state = _query[i]->loci[j]->fill_geode->getOrCreateStateSet();
                        Material* mat = dynamic_cast<Material*>(state->getAttribute(StateAttribute::MATERIAL, 0));
                        mat->setAlpha(Material::FRONT_AND_BACK, 0.4);
                    }
                }
            }
        }
    }

    if (menuItem == _scaleBar)
    {   /*
           if (_scaleBar->getValue())
           {
               osg::Matrix w2l = PluginHelper::getWorldToObjectTransform();
               osg::Vec3d start(0, 0, 0);
               start = start * w2l;
               cout << start.x() << " " << start.y() << " " << start.z() << "\n";
               std::cerr << selectArtifactSelected() << "\n";
               loadScaleBar(start);
           }
           else
           {
               _root->removeChild(_scaleBarModel);
           }
        */
//bangHand

        Matrixd camMat = PluginHelper::getHandMat(0);
        float cscale = 1; //PluginHelper::getObjectScale();
        Vec3 camTrans = camMat.getTrans();
        Quat camQuad = camMat.getRotate();
        cerr << "Hand Set Matrix: " << "Hands:" << PluginHelper::getNumHands() << " " << (camTrans.x() / cscale) << "," << (camTrans.y() / cscale) << "," << (camTrans.z() / cscale) << " Scale:" << cscale << " Rot:" << camQuad.x() << "," << camQuad.y() << "," << camQuad.z() << "\n";

        double x, y, z, rx, ry, rz;
        x = y = z = rx = ry = rz = 0.0;
        double bscale;
        //bscale = _flyplace->scale[i];
        x=-2231700;
        y=-4090410;
        z=-81120.3;
        x=739937.21259221 - 700000;
        y=3373215.8156312 - 3300000;
        z=1451.197693212;
        //x = 5
        //y = 5;
        //z = 5;
        //rx = 5;
        //ry = 5;
        //rz = 5;
        //-2.2317e+06,-4.09041e+06,-81120.3 Scale:55.8708
        Vec3 trans = Vec3(x, y, z);
        Matrix tmat;
        tmat.makeTranslate(trans);
        Vec3 xa = Vec3(1.0, 0.0, 0.0);
        Vec3 ya = Vec3(0.0, 1.0, 0.0);
        Vec3 za = Vec3(0.0, 0.0, 1.0);
        Matrix rot;
        rot.makeRotate(rx, xa, ry, ya, rz, za);
        Matrixd gotoMat = rot * tmat;
        //PluginHelper::setHandMat(0,gotoMat);
        //PluginHelper::setObjectScale(bscale);
        camMat = PluginHelper::getHandMat(0);
        cscale = 1; //PluginHelper::getObjectScale();
        camTrans = camMat.getTrans();
        camQuad = camMat.getRotate();
        cerr << "Hand Set Matrix: " << (camTrans.x() / cscale) << "," << (camTrans.y() / cscale) << "," << (camTrans.z() / cscale) << " Scale:" << cscale << " Rot:" << camQuad.x() << "," << camQuad.y() << "," << camQuad.z() << "\n";
    }

    for (int i = 0; i < _goto.size(); i++)
    {
        if (menuItem == _goto[i])
        {
		flyTo(i);
        }
    }

    if (menuItem == _createAnnotations)
    {
      /*
            if (_createAnnotations->getValue())
            {
                _createAnnotations->setValue(false);
                menuCallback(_selectCB);
            }
      */
    }
    if (menuItem == _bookmarkLoc)
    {
        //Matrixd camMat = PluginHelper::getHeadMat();
        Matrixd camMat = PluginHelper::getObjectMatrix();
        float cscale = PluginHelper::getObjectScale();
        Vec3 camTrans = camMat.getTrans();
        Quat camQuad = camMat.getRotate();
        //<placemark><name>Dam View 2</name><scale>1.47141</scale><x>12569.2</x><y>17333.1</y><z>-42045.8</z><rx>0.0266129</rx><ry>-0.0024595</ry><rz>-0.491622</rz><rw>0.870398</rw></placemark>
        cerr << "<placemark><name></name><scale>" << cscale << "</scale><x>" << (camTrans.x() / cscale) << "</x><y>" << (camTrans.y() / cscale) << "</y><z>" << (camTrans.z() / cscale) << "</z><rx>" << camQuad.x() << "</rx><ry>" << camQuad.y() << "</ry><rz>" << camQuad.z() << "</rz><rw>" << camQuad.w() << "</rw></placemark>\n";

        saveBookmark(camMat,cscale);
    }


    if (menuItem == _selectArtifactCB)
    {
        if (_selectArtifactCB->getValue())
        {
            if (_selectCB->getValue())
            {
                _selectCB->setValue(false);
                menuCallback(_selectCB);
            }
        }

       // _artifactPanel->setVisible(_selectArtifactCB->getValue());

        if (!_selectArtifactCB->getValue()) //New Add
        {
            _root->removeChild(_selectModelLoad.get());
        }
    }


    if (menuItem == _selectCB)
    {
        if (_selectCB->getValue())
        {
            if (_selectArtifactCB->getValue())
            {
                _selectArtifactCB->setValue(false);
                menuCallback(_selectArtifactCB);
            }

            for (int q = 0; q < _query.size(); q++)
            {
                vector<Artifact*> artifacts = _query[q]->artifacts;

                for (int i = 0; i < artifacts.size(); i++)
                {
                    artifacts[i]->selected = false;
                    osg::ShapeDrawable* sd = dynamic_cast<osg::ShapeDrawable*>(artifacts[i]->drawable);

                    if (sd)
                    {
                        osg::Vec4 color = sd->getColor();
                        color.x() = color.x() * 0.5;
                        color.y() = color.y() * 0.5;
                        color.z() = color.z() * 0.5;
                        sd->setColor(color);
                    }
                }
            }

            _selectStart = osg::Vec3(0, 0, 0);
            _selectCurrent = osg::Vec3(0, 0, 0);
            _root->addChild(_selectBox);

            if (PluginHelper::getNumHands())
            {
                PluginHelper::getScene()->addChild(_selectMark);
            }

            _selectionStatsPanel->setVisible(true);
        }
        else
        {
            for (int q = 0; q < _query.size(); q++)
            {
                vector<Artifact*> artifacts = _query[q]->artifacts;

                for (int i = 0; i < artifacts.size(); i++)
                {
                    if (!artifacts[i]->selected)
                    {
                        osg::ShapeDrawable* sd = dynamic_cast<osg::ShapeDrawable*>(artifacts[i]->drawable);

                        if (sd)
                        {
                            osg::Vec4 color = sd->getColor();
                            color.x() = color.x() * 2.0;
                            color.y() = color.y() * 2.0;
                            color.z() = color.z() * 2.0;
                            sd->setColor(color);
                        }
                    }
                }
            }

            _root->removeChild(_selectBox);

            if (PluginHelper::getNumHands())
            {
                PluginHelper::getScene()->removeChild(_selectMark);
            }

            _selectionStatsPanel->setVisible(false);
        }

        _selectActive = false;
    }
}
//Removes all conditions set in the query for the selected table.
void ArtifactVis2::clearConditions(Table* t)
{
    std::vector<cvr::MenuCheckbox*>::iterator button;

    for (button = t->querySlider.begin(); button < t->querySlider.end(); button++)
        (*button)->setValue(false);

    for (int i = 0; i < t->queryOptions.size(); i++)
        t->queryOptions[i]->setValue(t->queryOptions[i]->firstOn(), false);
}
//Converts the DC into a unique integer, 0 through (26^2 - 1).
int ArtifactVis2::dc2Int(string dc)
{
    char letter1 = dc.c_str() [0];
    char letter2 = dc.c_str() [1];
    int char1 = letter1 - 65;
    int char2 = letter2 - 65;
    int tot = char1 * 26 + char2;
    return tot;
}
std::string ArtifactVis2::getTimeModified(std::string file)
{
    struct tm* clock;
    struct stat attrib;
    stat(file.c_str(), &attrib);
    clock = gmtime(& (attrib.st_mtime));
    stringstream ss;
    ss << clock->tm_year + 1900;

    if (clock->tm_yday + 1 < 100) ss << "0";

    if (clock->tm_yday + 1 < 10) ss << "0";

    ss << clock->tm_yday + 1;

    if (clock->tm_hour < 10) ss << "0";

    ss << clock->tm_hour;

    if (clock->tm_min < 10) ss << "0";

    ss << clock->tm_min;

    if (clock->tm_sec < 10) ss << "0";

    ss << clock->tm_sec;
    string output = ss.str();
    return output;
}
void ArtifactVis2::preFrame()
{
if(ArtifactVis2On)
{
updateAnnoLine();
updateArtifactLine();
updateArtifactModel();
updateLineGroup();
}
    /*
    std::vector<Artifact*> allArtifacts;
        for (int i = 0; i < _query.size(); i++)
        {
            if (_queryDynamicUpdate[i]->getValue())
            {
                printf("Update On\n");
    		string path = _query[i]->kmlPath;
                string newTime = getTimeModified(path);

                if (newTime != _query[i]->timestamp)
                {
                    cout << "New query found for " << _query[i]->name << "." << endl;
                    readQuery(_query[i]);

                    if (_queryOption[i]->getValue())
                    {
                        if (_query[i]->sf)
                            displayArtifacts(_query[i]);

                        _query[i]->updated = false;
                        _root->addChild(_query[i]->sphereRoot);
                    }
                }
            }

            if (_query[i]->active && _query[i]->sf)
    	{
                for (int j = 0; j < _query[i]->artifacts.size(); j++)
                    allArtifacts.push_back(_query[i]->artifacts[j]);
    	printf("Query Active\n");
    	}

        }
    */
/*
    if (_selectCB->getValue())
    {
        updateSelect();
    }
*/


}
void ArtifactVis2::loadScaleBar(osg::Vec3d start)
{
int i = 0;
 string currentModelPath = _models3d[i]->fullpath;
 string name = _models3d[i]->name;
 newSelectedFile = "";
 newSelectedName = "";

// Matrix handMat = getHandToObjectMatrix();
         Vec3 currentPos = _models3d[i]->pos;
        Quat  currentRot = _models3d[i]->rot;
  //Check if ModelPath has been loaded
  Node* modelNode;
  
            if (objectMap.count(currentModelPath) == 0)
	    {
		 objectMap[currentModelPath] = osgDB::readNodeFile(currentModelPath);
	    }
            modelNode = objectMap[currentModelPath];
  
//Add Lighting and Culling

		if(false)
		{
		    osg::StateSet* stateset = modelNode->getOrCreateStateSet();
		    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
		}
		if(true)
		{
		    osg::StateSet * stateset = modelNode->getOrCreateStateSet();
		    osg::CullFace * cf=new osg::CullFace();
		    cf->setMode(osg::CullFace::BACK);
		    stateset->setAttributeAndModes( cf, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
		}
                if(true)
		{
		TextureResizeNonPowerOfTwoHintVisitor tr2v(false);
		modelNode->accept(tr2v);
                }
                if(false)
                {
                    StateSet* ss = modelNode->getOrCreateStateSet();
                    ss->setMode(GL_LIGHTING, StateAttribute::ON | osg::StateAttribute::OVERRIDE);
                    Material* mat = new Material();
                    mat->setColorMode(Material::AMBIENT_AND_DIFFUSE);
                    Vec4 color_dif(1, 1, 1, 1);
                    mat->setDiffuse(Material::FRONT_AND_BACK, color_dif);
                    ss->setAttribute(mat);
                    ss->setAttributeAndModes(mat, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
                }

//Add to SceneObject
  //   _query[q]->artifacts[inc]->model->name = basket;
    
float currentScale = _models3d[i]->scale;

	    SceneObject * so;
	    so = new SceneObject(name, false, false, false, true, false);
	    osg::Switch* switchNode = new osg::Switch();
	    so->addChild(switchNode);
	    PluginHelper::registerSceneObject(so,"Test");
	    so->attachToScene();
//Add currentNode to switchNode
      _models3d[i]->currentModelNode = modelNode;  
	switchNode->addChild(modelNode);
      _models3d[i]->switchNode = switchNode;

     //_root->addChild(modelNode);
//Add menu system
	    so->setNavigationOn(true);
	    so->setMovable(false);
	    so->addMoveMenuItem();
	    so->addNavigationMenuItem();
            float min = 0.0001;
            float max = 1;
            so->addScaleMenuItem("Scale",min,max,currentScale);
	    SubMenu * sm = new SubMenu("Position");
	    so->addMenuItem(sm);

	    MenuButton * mb;
	    mb = new MenuButton("Load");
	    mb->setCallback(this);
	    sm->addItem(mb);

	    SubMenu * savemenu = new SubMenu("Save");
	    sm->addItem(savemenu);

	    mb = new MenuButton("Save");
	    mb->setCallback(this);
	    savemenu->addItem(mb);
            _models3d[i]->saveMap = mb;

	    mb = new MenuButton("Save New Kml");
	    mb->setCallback(this);
	    savemenu->addItem(mb);
            _models3d[i]->saveNewMap = mb;

	    mb = new MenuButton("Reset to Origin");
	    mb->setCallback(this);
	    so->addMenuItem(mb);
            _models3d[i]->resetMap = mb;

            MenuCheckbox * mc;
	    mc = new MenuCheckbox("Active",false);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            _models3d[i]->activeMap = mc;

            
	    mc = new MenuCheckbox("Visible",true);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            _models3d[i]->visibleMap = mc;
            _models3d[i]->visible = true;

            float rValue = 0;
            min = -1;
            max = 1;
            MenuRangeValue* rt = new MenuRangeValue("rx",min,max,rValue);
            rt->setCallback(this);
	    so->addMenuItem(rt);
            _models3d[i]->rxMap = rt;

            rt = new MenuRangeValue("ry",min,max,rValue);
            rt->setCallback(this);
	    so->addMenuItem(rt);
            _models3d[i]->ryMap = rt;

            rt = new MenuRangeValue("rz",min,max,rValue);
            rt->setCallback(this);
	    so->addMenuItem(rt);
            _models3d[i]->rzMap = rt;
/*
	    mc = new MenuCheckbox("Panel Visible",true);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
 //           _query[q]->artifacts[inc]->model->pVisibleMap = mc;
           // _query[q]->artifacts[inc]->model->pVisible = true;
*/
Vec3 orig = currentPos; 
cerr << "Pos: " << orig.x() << " " << orig.y() << " " << orig.z() << "\n";

 so->setPosition(currentPos);     
 so->setScale(currentScale);
 so->setRotation(currentRot);     



    _models3d[i]->so = so;
    _models3d[i]->pos = so->getPosition();
    _models3d[i]->rot = so->getRotation();
    _models3d[i]->active = false;
    _models3d[i]->loaded = true;
}
void ArtifactVis2::setActiveArtifact(int _lockedTo, int _lockedType, int art, int q)
{
    vector<Artifact*> artifacts = _query[q]->artifacts;
    artifacts[art]->lockedTo = _lockedTo;
    artifacts[art]->lockedType = _lockedType;

    if (art < 0 || art >= artifacts.size())
    {
        return;
    }

    //cout << "Active Artifact: ";
    //cout << artifacts[art]->modelPos[0] << " " << artifacts[art]->modelPos[1] << " " << artifacts[art]->modelPos[2] << endl;

    if (art == _activeArtifact)
    {
       // return;
    }

    std::stringstream ss;

    for (int i = 0; i < artifacts[art]->fields.size(); i++)
    {
        ss << artifacts[art]->fields.at(i) << " " << artifacts[art]->values.at(i) << endl;
    }

    ss << "Position: " << endl;
    ss << "-Longitude: " << artifacts[art]->pos[0] << endl;
    ss << "-Latitude: " << artifacts[art]->pos[1] << endl;
    ss << "-Altitude: " << artifacts[art]->pos[2] << endl;
//    _artifactPanel->updateTabWithText("Info", ss.str());
    //Generate New Detail Graph
    createArtifactPanel(q,art,ss.str());
    createArtifactModel(q, art, "");
/*
    if (art == _activeArtifact)
    {
        return;
    }


    string picPath = ConfigManager::getEntry("Plugin.ArtifactVis2.PicFolder").append("photos/");
    string side = (picPath + artifacts[art]->values[1] + "/" + "SF.jpg");
    string top = (picPath + artifacts[art]->values[1] + "/" + "T.jpg");
    string bottom = (picPath + artifacts[art]->values[1] + "/" + "B.jpg");
    string check = top;

    if (!modelExists(check.c_str()))
    {
        //side =  (picPath+"50563_s.jpg");
        //top = (picPath+"50563_t.jpg");
        //bottom = (picPath+"50563_b.jpg");
    }

    cout << check << "\n";
    cout << top << "\n";
    _artifactPanel->updateTabWithTexture("Side", side);
    _artifactPanel->updateTabWithTexture("Top", top);
    _artifactPanel->updateTabWithTexture("Bottom", bottom);
    //std::cerr << "Side texture: " << side.str() << std::endl;
    //std::cerr << "Top texture: " << top.str() << std::endl;
    //std::cerr << "Bottom texture: " << bottom.str() << std::endl;

    // XXX models not disappearing
    //_root->removeChild(_selectModelLoad.get());
*/
}

void ArtifactVis2::readQuery(QueryGroup* query)
{
    _root->removeChild(query->sphereRoot);
    query->updated = true;
    std::vector<Artifact*> generatedArtifacts;
    string filename = query->kmlPath;
    cerr << "Reading query: " << filename << endl;
    FILE* fp;
    mxml_node_t* tree;
    fp = fopen(filename.c_str(), "r");

    if (fp == NULL)
    {
        std::cerr << "Unable to open file: " << filename << std::endl;
        return;
    }

    tree = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
    fclose(fp);

    if (tree == NULL)
    {
        std::cerr << "Unable to parse XML file: " << filename << std::endl;
        return;
    }

    mxml_node_t* node;
    node = mxmlFindElement(tree, tree, "name", NULL, NULL, MXML_DESCEND);
    query->name = node->child->value.text.string;
    node = mxmlFindElement(tree, tree, "query", NULL, NULL, MXML_DESCEND);
    query->query = node->child->value.text.string;
    node = mxmlFindElement(tree, tree, "timestamp", NULL, NULL, MXML_DESCEND);
    query->timestamp = getTimeModified(filename);

    if (!query->sf)
    {
        readLocusFile(query);
        cout << "Query read!" << endl;
        return;
    }

    for (node = mxmlFindElement(tree, tree, "Placemark", NULL, NULL, MXML_DESCEND); node != NULL; node = mxmlFindElement(node, tree, "Placemark", NULL, NULL, MXML_DESCEND))
    {
        Artifact* newArtifact = new Artifact();
        newArtifact->patmt = new PositionAttitudeTransform();
        newArtifact->rt = new MatrixTransform();
        newArtifact->scalet = new MatrixTransform();
        //newArtifact->rt->addChild(newArtifact->scalet);
        //newArtifact->patmt->addChild(newArtifact->rt);
        newArtifact->label = new osgText::Text();
        mxml_node_t* desc_node = mxmlFindElement(node, tree, "description", NULL, NULL, MXML_DESCEND);
        mxml_node_t* desc_child;
        string dc;
        string basket;

        for (desc_child = desc_node->child; desc_child != NULL; desc_child = desc_child->next)
        {
            char* desc_text = desc_child->value.text.string;
            string desc = desc_text;

            if (desc.find(":", 0) != string::npos)
            {
                if (desc.find("dccode:", 0) != string::npos) dc = desc_child->next->value.text.string;

                if (desc.find("d_code:", 0) != string::npos) dc = desc_child->next->value.text.string;

                if (desc.find("the_geom:", 0) != string::npos)
                {
                    string coord;

                    for (int i = 0; i < 3; i++)
                    {
                        desc_child = desc_child->next;
                        std::istringstream ss;
                        //std::cout.precision(11);
                        coord = desc_child->value.text.string;
                        //coord = coord.erase(coord.find(".")+4);
                        //pos[i] = atof(coord.c_str());
                        ss.str(coord);
                        ss >> newArtifact->pos[i];
                    }
                }
                else
                {
                    string value_text = desc_child->next->value.text.string;

                    if (desc.find("basket", 0) != string::npos)
                        basket = value_text;

                    newArtifact->fields.push_back(desc);

                    if (value_text.find("NULL", 0) == string::npos)
                        newArtifact->values.push_back(value_text);
                    else
                        newArtifact->values.push_back("-");
                }
            }
        }

        newArtifact->dc = dc;
        newArtifact->label->setText(basket + dc.substr(0, 2));
        newArtifact->visible = true;
        newArtifact->lockedTo = -1;
        newArtifact->lockedType = -1;
        generatedArtifacts.push_back(newArtifact);
    }
    cout << "Artifacts Loaded: " << generatedArtifacts.size() << "\n";
    query->artifacts = generatedArtifacts;
    cout << "Query read!" << endl;
}
void ArtifactVis2::listArtifacts()
{
    for (int q = 0; q < _query.size(); q++)
    {
        vector<Artifact*> artifacts = _query[q]->artifacts;
        cerr << "Listing " << artifacts.size() << " elements:" << endl;
        vector<Artifact*>::iterator item = artifacts.begin();

        for (; item < artifacts.end(); item++)
        {
            for (int i = 0; i < (*item)->fields.size(); i++)
            {
                cout << (*item)->fields.at(i) << " " << (*item)->values.at(i) << " ";
            }

            cout << "Position: " << (*item)->pos[0] << ", " << (*item)->pos[1] << ", " << (*item)->pos[2];
            cout << endl;
        }
    }
}
bool ArtifactVis2::modelExists(const char* filename)
{
    ifstream ifile(filename);
    return !ifile.fail();
}
void ArtifactVis2::displayArtifacts(QueryGroup* query)
{
    Group* root_node = query->sphereRoot;

    while (root_node->getNumChildren() != 0)
    {
        root_node->removeChild(root_node->getChild(0));
    }

    const double M_TO_MM = 1.0f;
    //const double LATLONG_FACTOR = 100000.0f;
    std::vector<Artifact*> artifacts = query->artifacts;
    cerr << "Creating " << artifacts.size() << " artifacts..." << endl;
    vector<Artifact*>::iterator item = artifacts.begin();
    float tessellation = ConfigManager::getFloat("Plugin.ArtifactVis2.Tessellation", .2);
    Vec3d offset = Vec3d(
                       ConfigManager::getFloat("Plugin.ArtifactVis2.Offset.X", 0),
                       ConfigManager::getFloat("Plugin.ArtifactVis2.Offset.Y", 0),
                       ConfigManager::getFloat("Plugin.ArtifactVis2.Offset.Z", 0));
    //cerr << "Coords: " << offset.x() << "," << offset.y() << "\n";
    osg::Geode* sphereGeode = new osg::Geode();
    Vec3d center(0, 0, 0);

    for (; item < artifacts.end(); item++)
    {
        Vec3d position((*item)->pos[0], (*item)->pos[1], (*item)->pos[2]);
        osg::Vec3d pos;

        if (!_ossim)
        {
            Matrixd trans;
            trans.makeTranslate(position);
            Matrixd scale;
            scale.makeScale(M_TO_MM, M_TO_MM, M_TO_MM);
            //scale.makeScale(M_TO_MM*LATLONG_FACTOR, M_TO_MM*LATLONG_FACTOR, M_TO_MM);
            //Matrixd rot1;
            //rot1.makeRotate(osg::DegreesToRadians(-90.0), 0, 1, 0);
            //Matrixd rot2;
            //rot2.makeRotate(osg::DegreesToRadians(90.0), 1, 0, 0);
            //Matrixd mirror;
            //mirror.makeScale(1, -1, 1);
            Matrixd offsetMat;
            offsetMat.makeTranslate(offset);
            //pos = osg::Vec3d(0,0,0) * mirror * trans * scale * mirror * rot2 * rot1 * offsetMat;
            pos = osg::Vec3d(0, 0, 0) * trans * offsetMat;
            //pos = position;
            //printf("artifact %f %f %f\n", pos[0], pos[1], pos[2]);
            (*item)->modelPos = pos;
            (*item)->modelOriginalPos = pos;

            //center+=pos;
        }
        else
        {
            pos = position;
            //center+=pos;
            (*item)->modelPos = pos;
        }
    }

    //center/=artifacts.size();
    //bango
    for (item = artifacts.begin(); item < artifacts.end(); item++)
    {
        //cerr<<"Creating object "<<(item-artifacts.begin())<<" out of "<<artifacts.size()<<endl;
        if (_ossim)
        {
            // (*item)->modelPos-=center;
        }

        Vec3d pos = (*item)->modelPos;
        int dcInt = dc2Int((*item)->dc);

        if (!_modelLoaded[dcInt])
            // if(true)
        {
            //            osg::Drawable* g = createObject((*item)->dc, tessellation, Vec3(0,0,0));
            //            g->setUseDisplayList(false);
            //            (*item)->drawable = g;
            //            sphereGeode->addDrawable((*item)->drawable);
            //
            //            (*item)->patmt->setPosition(pos);
            //            (*item)->patmt->addDrawable((*item)->drawable);
            //Vec3d* vvv = new Vec3d(0, 0, 0);

            osg::Drawable* g = createObject((*item)->dc, tessellation, pos);
            g->setUseDisplayList(false);
            (*item)->drawable = g;
            sphereGeode->addDrawable((*item)->drawable);
            (*item)->patmt->setPosition(pos);
            (*item)->rt->addChild((*item)->scalet);
            (*item)->patmt->addChild((*item)->rt);
            (*item)->scalet->addChild(sphereGeode);
            //root_node->addChild((*item)->patmt);
        }
        else
        {
            //            PositionAttitudeTransform* modelTrans = new PositionAttitudeTransform();
            Matrixd scale;
            double snum = 0.05;
            //            scale.makeScale(snum, snum, snum);
            (*item)->setScale(snum);
            //(*item)->scalet = new MatrixTransform();
            //(*item)->scalet->setMatrix(scale);
            //string modelPath = "/home/ngsmith/ArchInterface/data/dcode_models/Finished/obj/LW/LW.obj";
            //osgDB::readNodeFile(modelPath)

            (*item)->scalet->addChild(_models[dcInt]);
            // (*item)->scalet->addChild(osgDB::readNodeFile(modelPath));
            (*item)->patmt->setPosition(pos);
            (*item)->rt->addChild((*item)->scalet);
            (*item)->patmt->addChild((*item)->rt);
            //(*item)->patmt->addChild((*item)->scalet);
            root_node->addChild((*item)->patmt);
            //(*item)->drawable = (*item)->label;
            //sphereGeode->addDrawable((*item)->patmt);
        }

        sphereGeode->addDrawable((*item)->label);
        (*item)->label->setUseDisplayList(false);
        (*item)->label->setAxisAlignment(osgText::Text::SCREEN);
        (*item)->label->setPosition((*item)->modelPos + Vec3f(0, 0, _sphereRadius * 1.1));
        (*item)->label->setAlignment(osgText::Text::CENTER_CENTER);
        (*item)->label->setCharacterSize(15);
        (*item)->label->setCharacterSizeMode(osgText::Text::SCREEN_COORDS);
    }
    cerr << "Items: " << artifacts.size() << "\n";
    //query->center = center;
    cerr << "done" << endl;
    StateSet* ss = sphereGeode->getOrCreateStateSet();
    //ss->setMode(GL_BLEND, StateAttribute::ON);
    ss->setMode(GL_LIGHTING, StateAttribute::ON);
    ss->setRenderingHint(osg::StateSet::OPAQUE_BIN);
    ss->setAttribute(_defaultMaterial);
    osg::CullFace* cf = new osg::CullFace();
    cf->setMode(osg::CullFace::BACK);
    ss->setAttributeAndModes(cf, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    // cout << center.x() << ", " << center.y() << endl;
    if (_ossim)
    {
#ifdef WITH_OSSIMPLANET
        OssimPlanet::instance()->addModel(sphereGeode, center.y(), center.x(), Vec3(1.0, 1.0, 1.0), 10, 0, 0, 0);
#endif
    }
    else
    {
        root_node->addChild(sphereGeode);
    }

}

osg::Drawable* ArtifactVis2::createObject(std::string dc, float tessellation, Vec3d& pos)
{
    TessellationHints* hints = new TessellationHints();
    hints->setDetailRatio(tessellation);
    Box* sphereShape = new Box(pos, _sphereRadius);
    ShapeDrawable* shapeDrawable = new ShapeDrawable(sphereShape);
    shapeDrawable->setTessellationHints(hints);
    shapeDrawable->setColor(_colors[dc2Int(dc)]);
    return shapeDrawable;
}
void ArtifactVis2::readPointCloud(int index)
{
    Vec3Array* coords;
    Vec4Array* colors;
    string filename = ConfigManager::getEntry("Plugin.ArtifactVis2.3DModelFolder").append(_showPCCB[index]->getText()); //Replaced
    string type = filename;

    bool nvidia = ConfigManager::getBool("Plugin.ArtifactVis2.Nvidia");
    if (!_coordsPC[index])
    {
        coords = new Vec3Array();
        colors = new Vec4Array();
        cout << "Reading point cloud data from : " << filename <<  "..." << endl;
        ifstream file(filename.c_str());
        type.erase(0, (type.length() - 3));
        cout << type << "\n";
        string line;
        bool read = false;
        cout << _pcFactor[index] << "\n";
       // int factor = _pcFactor[index];
        int factor = 1;
        if((type == "ply" || type == "xyb") && nvidia)
        {
            cout << "Reading XYB" << endl;

            //float x,y,z;
            //bamop
            float scale = 9;
            //	osg::Vec3 offset(transx, transy, transz);
            //_activeObject = new PointsObject("Points",true,false,false,true,true);


            cout << "File: " << filename << endl;
            PointsLoadInfo pli;
            pli.file = filename;
            pli.group = new osg::Group();

            PluginHelper::sendMessageByName("Points",POINTS_LOAD_REQUEST,(char*)&pli);

            if(!pli.group->getNumChildren())
            {
                std::cerr << "PointsWithPans: Error, no points loaded for file: " << pli.file << std::endl;
                return;
            }

            float pscale;
            if(type == "xyb")
            {
                pscale = 100;
            }
            else
            {
                pscale = 0.3;
            }
            pscale = ConfigManager::getFloat("Plugin.ArtifactVis2.scaleP", 0);
            cerr << "Scale of P is " << pscale << "\n";

            osg::Uniform*  _scaleUni = new osg::Uniform("pointScale",1.0f * pscale);
            pli.group->getOrCreateStateSet()->addUniform(_scaleUni);
            //_activeObject->attachToScene();
            //_activeObject->setNavigationOn(false);
            //_activeObject->setScale(scale);
            //_activeObject->setPosition(_pcPos[index]);
            //_activeObject->setNavigationOn(true);
            //_activeObject->addChild(pli.group.get());

            MatrixTransform* rotTransform = new MatrixTransform();
            Matrix pitchMat;
            Matrix yawMat;
            Matrix rollMat;
            pitchMat.makeRotate(DegreesToRadians(_pcRot[index].x()), 1, 0, 0);
            yawMat.makeRotate(DegreesToRadians(_pcRot[index].y()), 0, 1, 0);
            rollMat.makeRotate(DegreesToRadians(_pcRot[index].z()), 0, 0, 1);
            rotTransform->setMatrix(pitchMat * yawMat * rollMat);
            MatrixTransform* scaleTransform = new MatrixTransform();
            Matrix scaleMat;
            scaleMat.makeScale(_pcScale[index]);
            scaleTransform->setMatrix(scaleMat);
            MatrixTransform* posTransform = new MatrixTransform();
            Matrix posMat;
            Matrix invertMat;
            posMat.makeTranslate(_pcPos[index]);
            posTransform->setMatrix(posMat);
            MatrixTransform* invTransform = new MatrixTransform();
            rotTransform->addChild(pli.group.get());
            cerr << "Num childs " << pli.group.get()->getNumChildren() << "\n";
            scaleTransform->addChild(rotTransform);
            posTransform->addChild(scaleTransform);

            _pcRoot[index] = new MatrixTransform();
            _pcRoot[index]->addChild(posTransform);
            cout << "Loaded" << endl;
            cerr << "Num childs2 " << _pcRoot[index]->getNumChildren() << "\n";
        }
        else
        {
            if (file.is_open())
            {
                int pcount = 0;
                int bten = factor - 1;
                int cptx = 0;
                double transx = 0;
                double transy = 0;
                double transz = 0;

                while (file.good())
                {
                    getline(file, line);
                    string lineout = line;

                    if (type == "ptx")
                    {
                        vector<string> entries;
                        //vector<string>array;
                        string token;
                        //string tmp = "this@is@a@line";
                        istringstream iss(line);
                        char lim = ' ';

                        while ( getline(iss, token, lim) )
                        {
                            entries.push_back(token);
                        }

                        if (entries.size() < 7)
                        {
                            cout << lineout << "\n";

                            if (cptx == 9)
                            {
                                cptx = 0;
                                //cout << lineout << "\n";
                                std::stringstream ss;
                                ss.precision(19);
                                transx = atof(entries[0].c_str());
                                transy = atof(entries[0].c_str());
                                transz = atof(entries[0].c_str());
                            }
                            else
                            {
                                cptx++;
                            }
                        }
                        else
                        {
                            std::stringstream ss;
                            ss.precision(19);
                            double x = atof(entries[0].c_str()) + transx;
                            double y = atof(entries[1].c_str()) + transy;
                            double z = atof(entries[2].c_str()) + transz;
                            double r = atof(entries[4].c_str()) / 255;
                            double g = atof(entries[5].c_str()) / 255;
                            double b = atof(entries[6].c_str()) / 255;
                            coords->push_back(Vec3d(x, y, z));
                            _avgOffset[index] += Vec3d(x, y, z);
                            colors->push_back(Vec4d(r, g, b, 1.0));
                            bten = 0;
                            pcount++;
                        }
                    }

                    if (line.empty()) break;

                    if (read)
                    {
                        bten++;
                        vector<string> entries;
                        int ck = 0;

                        /*
                        while(true)
                        {
                            if(line.find(" ")==line.rfind(" "))
                                break;
                            else
                            {
                                entries.push_back(line.substr(0,line.find(" ")));
                                line = line.substr(line.find(" ")+1);
                                cout << line << "\n";
                            }
                        }
                        */
                        if (bten == factor)
                        {
                            while (ck < 7)
                            {
                                entries.push_back(line.substr(0, line.find(" ")));
                                line = line.substr(line.find(" ") + 1);
                                //cout << line << "\n";
                                ck++;
                            }
                        }

                        //cout << entries.size();
                        if ((type == "ply") && (bten == factor))
                        {
                            float x = atof(entries[0].c_str());
                            float y = atof(entries[1].c_str());
                            float z = atof(entries[2].c_str());
                            float r = atof(entries[3].c_str()) / 255;
                            float g = atof(entries[4].c_str()) / 255;
                            float b = atof(entries[5].c_str()) / 255;
                            coords->push_back(Vec3d(x, y, z));
                            _avgOffset[index] += Vec3d(x, y, z);
                            colors->push_back(Vec4f(r, g, b, 1.0));
                            pcount++;
                            bten = 0;
                        }

                        if ((type == "txt") && (bten == factor))
                        {
                            float x = atof(entries[0].c_str());
                            float y = atof(entries[1].c_str());
                            float z = atof(entries[2].c_str());
                            float r = atof(entries[3].c_str()) / 255;
                            float g = atof(entries[4].c_str()) / 255;
                            float b = atof(entries[5].c_str()) / 255;
                            coords->push_back(Vec3d(x, y, z));
                            _avgOffset[index] += Vec3d(x, y, z);
                            colors->push_back(Vec4f(r, g, b, 1.0));
                            pcount++;
                            bten = 0;
                        }
                        else if ((type == "pts") && (bten == factor))
                        {
                            std::stringstream ss;
                            ss.precision(19);
                            double x = atof(entries[0].c_str());
                            double y = atof(entries[1].c_str());
                            double z = atof(entries[2].c_str());
                            double r = atof(entries[4].c_str()) / 255;
                            double g = atof(entries[5].c_str()) / 255;
                            double b = atof(entries[6].c_str()) / 255;
                            coords->push_back(Vec3d(x, y, z));
                            _avgOffset[index] += Vec3d(x, y, z);
                            colors->push_back(Vec4d(r, g, b, 1.0));
                            bten = 0;
                            pcount++;

                            if (pcount == 7216)
                            {
                                //std::stringstream ss;
                                //ss.precision(19);
                                printf("TOP %f %f %f\n", x, y, z);
                                cout << "Coords: " << entries[0].c_str() << " " << entries[1].c_str() << " " << entries[2].c_str() << "\n";
                                //cout << "Coords: " << coords[7216][0] << " " << coords[7216][1] << " " << coords[7216][2] << "\n";
                            }
                        }
                    }

                    if (type == "pts")
                    {
                        read = true;
                    }

                    if (line == "end_header")
                        read = true;
                }

                _coordsPC[index] = coords;
                _colorsPC[index] = colors;
                cout << "Finished Loading, Total Vertices: " << pcount << "\n";
                //cout << "Coords: " << coords<< " " << y << " " << z << "\n";
            }
            else
            {
                cout << "Unable to open file: " << filename << endl;
                return;
            }
        }
    }
    if(!nvidia)
    {
     //   coords = _coordsPC[index];
       // colors = _colorsPC[index];
        _avgOffset[index] /= coords->size();

        Geometry* pointCloud = new Geometry();
        Geode* pointGeode = new Geode();
        pointCloud->setVertexArray(coords);
        pointCloud->setColorArray(colors);
        pointCloud->setColorBinding(Geometry::BIND_PER_VERTEX);
        DrawElementsUInt* points = new DrawElementsUInt(PrimitiveSet::POINTS, 0);

        for (int i = 0; i < coords->size(); i++)
        {
            points->push_back(i);
        }

        pointCloud->addPrimitiveSet(points);
        pointGeode->addDrawable(pointCloud);
        StateSet* ss = pointGeode->getOrCreateStateSet();
        ss->setMode(GL_LIGHTING, StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
        if(true)
        {
        MatrixTransform* rotTransform = new MatrixTransform();
        Matrix pitchMat;
        Matrix yawMat;
        Matrix rollMat;
        pitchMat.makeRotate(DegreesToRadians(_pcRot[index].x()), 1, 0, 0);
        yawMat.makeRotate(DegreesToRadians(_pcRot[index].y()), 0, 1, 0);
        rollMat.makeRotate(DegreesToRadians(_pcRot[index].z()), 0, 0, 1);
        rotTransform->setMatrix(pitchMat * yawMat * rollMat);
        MatrixTransform* scaleTransform = new MatrixTransform();
        Matrix scaleMat;
        scaleMat.makeScale(_pcScale[index]);
        scaleTransform->setMatrix(scaleMat);
        MatrixTransform* posTransform = new MatrixTransform();
        Matrix posMat;
        Matrix invertMat;
        //invertMat.invert()
        posMat.makeTranslate(_pcPos[index]);
        posTransform->setMatrix(posMat);
        MatrixTransform* invTransform = new MatrixTransform();
        rotTransform->addChild(pointGeode);
        scaleTransform->addChild(rotTransform);
        posTransform->addChild(scaleTransform);
        //invertMat = posTransform->getInverseMatrix();
        //posTransform.invert(posTransform);
        //invTransform->setMatrix()
        _pcRoot[index] = new MatrixTransform();
        _pcRoot[index]->addChild(posTransform);
        //_pcRoot[index]->addChild(inversMat);
        //_pcRoot[index]->addChild(pointGeode);
        //cout << _pcRoot[index][5].x() << "\n";
        }
        if(false)
        {
//bangPC
//     19.6  18 89.23 
            string name = "test";
Vec3 arrScale = _pcScale[index];
float currentScale = arrScale.x();
	    SceneObject * so;
	    so = new SceneObject(name, false, false, false, true, false);
	    osg::Switch* switchNode = new osg::Switch();
	    so->addChild(switchNode);
	    PluginHelper::registerSceneObject(so,"Test");
	    so->attachToScene();
//Add currentNode to switchNode
     // _models3d[i]->currentModelNode = modelNode;  
	switchNode->addChild(pointGeode);
     // _pointClouds[i]->switchNode = switchNode;

     //_root->addChild(modelNode);
//Add menu system
	    so->setNavigationOn(true);
	    so->setMovable(true);
	    so->addMoveMenuItem();
	    so->addNavigationMenuItem();
            float min = 0.0001;
            float max = 1;
            so->addScaleMenuItem("Scale",min,max,currentScale);
	    SubMenu * sm = new SubMenu("Position");
	    so->addMenuItem(sm);

	    MenuButton * mb;
	    mb = new MenuButton("Load");
	    mb->setCallback(this);
	    sm->addItem(mb);

	    SubMenu * savemenu = new SubMenu("Save");
	    sm->addItem(savemenu);

	    mb = new MenuButton("Save");
	    mb->setCallback(this);
	    savemenu->addItem(mb);
          //  _pointClouds[i]->saveMap = mb;

	    mb = new MenuButton("Save New Kml");
	    mb->setCallback(this);
	    savemenu->addItem(mb);
           // _pointClouds[i]->saveNewMap = mb;

	    mb = new MenuButton("Reset to Origin");
	    mb->setCallback(this);
	    so->addMenuItem(mb);
           // _pointClouds[i]->resetMap = mb;

            MenuCheckbox * mc;
	    mc = new MenuCheckbox("Active",true);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
           // _pointClouds[i]->activeMap = mc;

            
	    mc = new MenuCheckbox("Visible",true);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
           // _pointCloudsi]->visibleMap = mc;
           // _pointClouds[i]->visible = true;

            float rValue = 0;
            min = -1;
            max = 1;
            MenuRangeValue* rt = new MenuRangeValue("rx",min,max,rValue);
            rt->setCallback(this);
	    so->addMenuItem(rt);
          //  _pointClouds[i]->rxMap = rt;

            rt = new MenuRangeValue("ry",min,max,rValue);
            rt->setCallback(this);
	    so->addMenuItem(rt);
          //  _pointClouds[i]->ryMap = rt;

            rt = new MenuRangeValue("rz",min,max,rValue);
            rt->setCallback(this);
	    so->addMenuItem(rt);
          //  _pointClouds[i]->rzMap = rt;
/*
	    mc = new MenuCheckbox("Panel Visible",true);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
 //           _query[q]->artifacts[inc]->model->pVisibleMap = mc;
           // _query[q]->artifacts[inc]->model->pVisible = true;
*/
//_pcRot[index].x()
float rotDegrees[3];
rotDegrees[0] = _pcRot[index].x();
rotDegrees[1] = _pcRot[index].y();
rotDegrees[2] = _pcRot[index].z();
Quat rot = osg::Quat(rotDegrees[0], osg::Vec3d(1,0,0),rotDegrees[1], osg::Vec3d(0,1,0),rotDegrees[2], osg::Vec3d(0,0,1)); 
Quat currentRot = rot;
Vec3 currentPos = _pcPos[index];

//     19.6  18 89.23 


Vec3 orig = currentPos; 
cerr << "Pos: " << orig.x() << " " << orig.y() << " " << orig.z() << "\n";

 so->setPosition(currentPos);     
 so->setScale(currentScale);
 so->setRotation(currentRot);     

orig = so->getPosition();
currentScale = so->getScale();
cerr << "So Pos: " << orig.x() << " " << orig.y() << " " << orig.z() << "\n";
cerr << "So Scale: " << currentScale << endl;
   // _pointClouds[i]->so = so;
   // _pointClouds[i]->pos = so->getPosition();
  //  _pointClouds[i]->rot = so->getRotation();
  //  _pointClouds[i]->active = true;
  //  _pointClouds[i]->loaded = true;
        }
    }
}
void ArtifactVis2::readSiteFile(int index)
{
    const double INCH_IN_MM = 25.4f;
    const double M_TO_MM = 1.0f;
    //std::string modelFileName = ConfigManager::getEntry("Plugin.ArtifactVis2.ArchInterfaceFolder").append("Model/").append(_showModelCB[index]->getText());
    std::string modelFileName = ConfigManager::getEntry("Plugin.ArtifactVis2.3DModelFolder").append(_showModelCB[index]->getText());   //Replaced
    cerr << "Reading site file: " << modelFileName << " " << index << " ..." << endl;

    if (!modelFileName.empty())
    {
        //cout << _modelSFileNode.size() << "\n";
        if (!_modelSFileNode[index])
        {
            _modelSFileNode[index] = osgDB::readNodeFile(modelFileName);
        }

        Node* modelFileNode = _modelSFileNode[index];

        if (modelFileNode == NULL) cerr << "Error reading file" << endl;
        else
        {
            MatrixTransform* siteScale = new MatrixTransform();
            Matrix scaleMat;
           // scaleMat.makeScale(_siteScale[index]*INCH_IN_MM);
            scaleMat.makeScale(_siteScale[index]);
            siteScale->setMatrix(scaleMat);
            siteScale->addChild(modelFileNode);

            if (_ossim)
            {
                _siteRoot[index]->addChild(siteScale);
#ifdef WITH_OSSIMPLANET
                OssimPlanet::instance()->addModel(_siteRoot[index], _sitePos[index].y(), _sitePos[index].x(), Vec3f(1, 1, 1), 0, 0, 0, 0);
#endif
                cout << _sitePos[index].y() << ", " << _sitePos[index].x() << endl;
            }
            /*
                        else if(_osgearth)
                        {
                    _siteRoot[index]->addChild(siteScale);
                            OsgEarthRequest request;
                        request.lat = _sitePos[index].y();
                        request.lon = _sitePos[index].x();
                        cout << "Lat, Lon: " << _sitePos[index].y() << ", " << _sitePos[index].x() << endl;
                        request.height = 30000.0f;
                        request.trans = _siteRoot[index];
                        PluginManager::instance()->sendMessageByName("OsgEarth",OE_ADD_MODEL,(char *) &request);

                        }
            */
            else
            {
                MatrixTransform* siteRot = new MatrixTransform();
                //Matrixd rot1;
                //rot1.makeRotate(osg::DegreesToRadians(-90.0), 0, 1, 0);
                //Matrixd rot2;
                //rot2.makeRotate(osg::DegreesToRadians(90.0), 1, 0, 0);
                Matrix pitchMat;
                Matrix yawMat;
                Matrix rollMat;
                pitchMat.makeRotate(DegreesToRadians(_siteRot[index].x()), 1, 0, 0);
                yawMat.makeRotate(DegreesToRadians(_siteRot[index].y()), 0, 1, 0);
                rollMat.makeRotate(DegreesToRadians(_siteRot[index].z()), 0, 0, 1);
                siteRot->setMatrix(pitchMat * yawMat * rollMat);
                //siteRot->setMatrix(rot2);
                siteRot->addChild(siteScale);
                MatrixTransform* siteTrans = new MatrixTransform();
                Matrix transMat;
                transMat.makeTranslate(_sitePos[index]);
                siteTrans->setMatrix(transMat);
                siteTrans->addChild(siteRot);
                _siteRoot[index] = new MatrixTransform();
                _siteRoot[index]->addChild(siteTrans);
            }

            StateSet* ss = _siteRoot[index]->getOrCreateStateSet();
            ss->setMode(GL_LIGHTING, StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            Material* mat = new Material();
            mat->setColorMode(Material::AMBIENT_AND_DIFFUSE);
            Vec4 color_dif(1, 1, 1, 1);
            mat->setDiffuse(Material::FRONT_AND_BACK, color_dif);
            ss->setAttribute(mat);
            ss->setAttributeAndModes(mat, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            cerr << "File read." << endl;
        }
    }
    else
    {
        cerr << "Error: Plugin.ArtifactVis2.Topo needs to point to a .wrl 3D topography file" << endl;
    }
}
void ArtifactVis2::readHudFile(int index)
{
    std::string modelFileName = ConfigManager::getEntry("Plugin.ArtifactVis2.3DModelFolder").append(_showHudCB[index]->getText());   //Replaced
    cerr << "Reading site file: " << modelFileName << " " << index << " ..." << endl;

    if (!modelFileName.empty())
    {
        //cout << _modelSFileNode.size() << "\n";
        if (!_hudFileNode[index])
        {
            _hudFileNode[index] = osgDB::readNodeFile(modelFileName);
        }

	    cerr << "Read\n";
        Node* hudFileNode = _hudFileNode[index];

        if (hudFileNode == NULL) cerr << "Error reading file" << endl;
        else
        {
	    cerr << "Reading\n";
            MatrixTransform* siteScale = new MatrixTransform();
            Matrix scaleMat;
            scaleMat.makeScale(_hudScale[index]);
            siteScale->setMatrix(scaleMat);
            siteScale->addChild(hudFileNode);

                MatrixTransform* siteRot = new MatrixTransform();
                Matrix pitchMat;
                Matrix yawMat;
                Matrix rollMat;
                pitchMat.makeRotate(DegreesToRadians(_hudRot[index].x()), 1, 0, 0);
                yawMat.makeRotate(DegreesToRadians(_hudRot[index].y()), 0, 1, 0);
                rollMat.makeRotate(DegreesToRadians(_hudRot[index].z()), 0, 0, 1);
                siteRot->setMatrix(pitchMat * yawMat * rollMat);
                siteRot->addChild(siteScale);
                MatrixTransform* siteTrans = new MatrixTransform();
//Here              
  Matrix transMat;
                transMat.makeTranslate(_hudPos[index]);
                siteTrans->setMatrix(transMat);
                siteTrans->addChild(siteRot);
                _hudRoot[index] = new MatrixTransform();
                _hudRoot[index]->setMatrix(transMat);
                _hudRoot[index]->addChild(siteRot);
/*  
          osg::Program* modelProgramObject = new osg::Program;
            string shaderPath = "/home/calvr/osgdata/shaders";
            string shaderVertexPath = ConfigManager::getEntry("Plugin.ArtifactVis2.ShaderVert");
            string shaderFragmentPath = ConfigManager::getEntry("Plugin.ArtifactVis2.ShaderFrag");
            modelProgramObject->addShader(osg::Shader::readShaderFile(osg::Shader::VERTEX, osgDB::findDataFile(shaderVertexPath)));
            modelProgramObject->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, osgDB::findDataFile(shaderFragmentPath)));
*/
        osg::Program *stProgram;
        osg::Shader *stVertObj;
        osg::Shader *stFragObj;

        osg::Texture2D *aocct = new osg::Texture2D();
        aocct->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    aocct->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        aocct->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    aocct->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    aocct->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

        osg::Image *occi = osgDB::readImageFile("/home/calvr/tutorials/AmbientOcc/images/AmbientOcc.bmp");
    aocct->setImage(occi);


        osg::StateSet *hebeState = new osg::StateSet();
        _hudRoot[index]->setStateSet(hebeState);

    hebeState->addUniform( new osg::Uniform("/home/calvr/tutorials/AmbientOcc/images/AmbientOcclusion", 0) );
        hebeState->setTextureAttribute(0, aocct);


        osg::TextureCubeMap *aoccm = new osg::TextureCubeMap();
    aoccm->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    aoccm->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    aoccm->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    aoccm->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
    aoccm->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        osg::Image *occbi = osgDB::readImageFile("/home/calvr/tutorials/AmbientOcc/images/px_diff.bmp");
    aoccm->setImage(osg::TextureCubeMap::POSITIVE_X, occbi);
        occbi = osgDB::readImageFile("/home/calvr/tutorials/AmbientOcc/images/nx_diff.bmp");
    aoccm->setImage(osg::TextureCubeMap::NEGATIVE_X, occbi);
    occbi = osgDB::readImageFile("/home/calvr/tutorials/AmbientOcc/images/ny_diff.bmp");
    aoccm->setImage(osg::TextureCubeMap::POSITIVE_Y, occbi);
        occbi = osgDB::readImageFile("/home/calvr/tutorials/AmbientOcc/images/py_diff.bmp");
    aoccm->setImage(osg::TextureCubeMap::NEGATIVE_Y, occbi);
        occbi = osgDB::readImageFile("/home/calvr/tutorials/AmbientOcc/images/pz_diff.bmp");
    aoccm->setImage(osg::TextureCubeMap::POSITIVE_Z, occbi);
        occbi = osgDB::readImageFile("/home/calvr/tutorials/AmbientOcc/images/nz_diff.bmp");
    aoccm->setImage(osg::TextureCubeMap::NEGATIVE_Z, occbi);

        hebeState->addUniform( new osg::Uniform("DiffuseEnvironment", 1) );
        hebeState->setTextureAttribute(1, aoccm);


        stProgram = new osg::Program;
    stProgram->setName( "statue" );
   // stVertObj = new osg::Shader( osg::Shader::VERTEX, osgDB::findDataFile("/home/calvr/tutorials/AmbientOcc/shaders/st.vert"));
   // stFragObj = new osg::Shader( osg::Shader::FRAGMENT, osgDB::findDataFile("/home/calvr/tutorials/AmbientOcc/shaders/st.frag"));
string shaderVertexPath = "/home/calvr/tutorials/AmbientOcc/shaders/st.vert";
string shaderFragmentPath = "/home/calvr/tutorials/AmbientOcc/shaders/st.frag";

    stProgram->addShader(osg::Shader::readShaderFile(osg::Shader::VERTEX, osgDB::findDataFile(shaderVertexPath)) );
    stProgram->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, osgDB::findDataFile(shaderFragmentPath)) );
    hebeState->setAttributeAndModes(stProgram, osg::StateAttribute::ON);
/*
            StateSet* ss = _hudRoot[index]->getOrCreateStateSet();
            bool useStd;
            useStd = false;
            if(useStd)
            {
            ss->setMode(GL_LIGHTING, StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            Material* mat = new Material();
            mat->setColorMode(Material::AMBIENT_AND_DIFFUSE);
            Vec4 color_dif(1, 1, 1, 1);
            mat->setDiffuse(Material::FRONT_AND_BACK, color_dif);
            ss->setAttribute(mat);
            ss->setAttributeAndModes(mat, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            }
            else
            {
            ss->setAttribute(modelProgramObject, osg::StateAttribute::ON);

        ss->addUniform(new osg::Uniform("LightPosition",osg::Vec3(0,0,200)));
        ss->addUniform(new osg::Uniform("SurfaceColor",osg::Vec3(0.75,0.75,0.75)));
        ss->addUniform(new osg::Uniform("WarmColor",osg::Vec3(0.6,0,0)));
        ss->addUniform(new osg::Uniform("CoolColor",osg::Vec3(0,0,0.6)));
        ss->addUniform(new osg::Uniform("DiffuseWarm",0.45f));
        ss->addUniform(new osg::Uniform("DiffuseCool", 0.45f));

            }
*/
            cerr << "File read." << endl;
        }
    }
    else
    {
        cerr << "Error: Plugin.ArtifactVis2.Topo needs to point to a .wrl 3D topography file" << endl;
    }
}

void ArtifactVis2::readLocusFile(QueryGroup* query)
{
    cout << "Reading Locus File..." << endl;
    const double M_TO_MM = 1.0f;
    //const double LATLONG_FACTOR = 100000.0f;
    Vec3d center(0, 0, 0);

    for (int i = 0; i < query->loci.size(); i++)
    {
        query->sphereRoot->removeChildren(0, query->sphereRoot->getNumChildren());
        delete query->loci[i];
    }

    query->loci.clear();
    Vec3f offset = Vec3f(
                       ConfigManager::getFloat("Plugin.ArtifactVis2.Offset.X", 0),
                       ConfigManager::getFloat("Plugin.ArtifactVis2.Offset.Y", 0),
                       ConfigManager::getFloat("Plugin.ArtifactVis2.Offset.Z", 0));
    std::string locusFile = query->kmlPath;

    if (locusFile.empty())
    {
        std::cerr << "ArtifactVis2: Warning: No Plugin.ArtifactVis2.LociFile entry." << std::endl;
        return;
    }

    FILE* fp;
    mxml_node_t* tree;
    fp = fopen(locusFile.c_str(), "r");

    if (fp == NULL)
    {
        std::cerr << "Unable to open file: " << locusFile << std::endl;
        return;
    }

    tree = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
    fclose(fp);

    if (tree == NULL)
    {
        std::cerr << "Unable to parse XML file: " << locusFile  << std::endl;
        return;
    }

    mxml_node_t* node = mxmlFindElement(tree, tree, "Placemark", NULL, NULL, MXML_DESCEND);

    for (; node != NULL; node = mxmlFindElement(node, tree, "Placemark", NULL, NULL, MXML_DESCEND))
    {
        Locus* loc = new Locus;
        mxml_node_t* desc_node = mxmlFindElement(node, tree, "description", NULL, NULL, MXML_DESCEND);
        mxml_node_t* desc_child;

        for (desc_child = desc_node->child; desc_child != NULL; desc_child = desc_child->next)
        {
            char* desc_text = desc_child->value.text.string;
            string desc = desc_text;

            if (desc.find(":", 0) != string::npos && desc.find("the_geom", 0) == string::npos)
            {
                if (desc.find("locus:", 0) != string::npos) loc->id = desc_child->next->value.text.string;

                string value_text = desc_child->next->value.text.string;
                loc->fields.push_back(desc);

                if (value_text.find("NULL", 0) == string::npos)
                    loc->values.push_back(value_text);
                else
                    loc->values.push_back("-");
            }
        }

        desc_node = mxmlFindElement(node, tree, "name", NULL, NULL, MXML_DESCEND);
        desc_child = desc_node->child;
        stringstream ss;
        ss << desc_child->value.text.string << " ";

        for (desc_child = desc_child->next; desc_child != NULL; desc_child = desc_child->next)
            ss << desc_child->value.text.string << " ";

        loc->name = ss.str();
        mxml_node_t* coord_node;
        //mxml_node_t * polyhedron_node;
        Vec3Array* coords = new Vec3Array();
        coord_node = mxmlFindElement(node, tree, "coordTop", NULL, NULL, MXML_DESCEND);
        mxml_node_t* child;
        Vec3d locCenter(0, 0, 0);

        for (child = coord_node->child; child != NULL; child = child->next)
        {
            // std::istringstream ss;
            //std::cout.precision(15);
            double pos[3];
            string coord;

            for (int i = 0; i < 3; i++)
            {
                //   ss.str(child->value.text.string);
                coord = child->value.text.string;
                //coord = coord.erase(coord.find(".")+4);
                pos[i] = atof(coord.c_str());

                if (i != 2)
                    child = child->next;
            }

            Vec3d position = Vec3d(pos[0], pos[1], pos[2]);

            if (_ossim)
            {
                loc->coordsTop.push_back(position);
                coords->push_back(position);
            }
            else
            {
                Matrixd scale;
                //scale.makeScale(M_TO_MM, M_TO_MM, M_TO_MM);
                //scale.makeScale(M_TO_MM*1000, M_TO_MM*1000, M_TO_MM*1000);
                Matrixd trans;
                trans.makeTranslate(position);
                //scale.makeScale(M_TO_MM*LATLONG_FACTOR, M_TO_MM*LATLONG_FACTOR, M_TO_MM);
                //Matrixd rot1;
                //rot1.makeRotate(osg::DegreesToRadians(-90.0), 0, 1, 0);
                //Matrixd rot2;
                //rot2.makeRotate(osg::DegreesToRadians(90.0), 1, 0, 0);
                //Matrixd mirror;
                //mirror.makeScale(1, -1, 1);
                Matrixd offsetMat;
                //offset = Vec3f(-700000,-3300000,0);
                offsetMat.makeTranslate(offset);
                osg::Vec3d posVec = osg::Vec3d(0, 0, 0) * trans * offsetMat ;
                //posVec[2] = -posVec[2];
                //osg::Vec3d posVec = position;
                //posVec[1] = -posVec[1];
                //printf("TOP %f %f %f\n", posVec[0], posVec[1], posVec[2]);
                loc->coordsTop.push_back(posVec);
                coords->push_back(posVec);
            }
        }

        coords->pop_back();
        coord_node = mxmlFindElement(node, tree, "coordBottom", NULL, NULL, MXML_DESCEND);

        for (child = coord_node->child; child != NULL; child = child->next)
        {
            // std::istringstream ss;
            //  std::cout.precision(15);
            double pos[3];
            string coord;

            for (int i = 0; i < 3; i++)
            {
                //  ss.str(child->value.text.string);
                coord = child->value.text.string;
                //coord = coord.erase(coord.find(".")+4);
                pos[i] = atof(coord.c_str());

                if (i != 2)
                    child = child->next;
            }

            Vec3d position = Vec3d(pos[0], pos[1], pos[2]);

            if (_ossim)
            {
                loc->coordsBot.push_back(position);
                coords->push_back(position);
            }
            else
            {
                Matrixd scale;
                scale.makeScale(M_TO_MM, M_TO_MM, M_TO_MM);
                //scale.makeScale(M_TO_MM*1000, M_TO_MM*1000, M_TO_MM*1000);
                Matrixd trans;
                trans.makeTranslate(position);
                //scale.makeScale(M_TO_MM*LATLONG_FACTOR, M_TO_MM*LATLONG_FACTOR, M_TO_MM);
                //Matrixd rot1;
                //rot1.makeRotate(osg::DegreesToRadians(-90.0), 0, 1, 0);
                //Matrixd rot2;
                //rot2.makeRotate(osg::DegreesToRadians(90.0), 1, 0, 0);
                //Matrixd mirror;
                //mirror.makeScale(1, -1, 1);
                Matrixd offsetMat;
                //  offset = Vec3f(-700000,-3300000,0);
                offsetMat.makeTranslate(offset);
                osg::Vec3d posVec = osg::Vec3d(0, 0, 0) * trans * scale * offsetMat ;
                //                osg::Vec3d posVec = position;
                //posVec[2] = -posVec[2];
                //posVec[1] = -posVec[1];
                //printf("BOTTOM %f %f %f\n", posVec[0], posVec[1], posVec[2]);
                loc->coordsBot.push_back(posVec);
                coords->push_back(posVec);
            }
        }

        coords->pop_back();
        int size = coords->size() / 2;

        if (size > 0)
        {
            Geometry* geom = new Geometry();
            Geometry* tgeom = new Geometry();
            Geode* fgeode = new Geode();
            Geode* lgeode = new Geode();
            Geode* tgeode = new Geode();
            geom->setVertexArray(coords);
            tgeom->setVertexArray(coords);

            for (int i = 0; i < size; i++)
            {
                DrawElementsUInt* face = new DrawElementsUInt(PrimitiveSet::QUADS, 0);
                face->push_back(i);
                face->push_back(i + size);
                face->push_back(((i + 1) % size) + size);
                face->push_back((i + 1) % size);
                geom->addPrimitiveSet(face);

                if (i < size - 1) //Commented out for now, adds caps to the polyhedra.
                {
                    face = new DrawElementsUInt(PrimitiveSet::TRIANGLES, 0);
                    face->push_back(0);
                    face->push_back(i);
                    face->push_back(i + 1);
                    geom->addPrimitiveSet(face);
                    tgeom->addPrimitiveSet(face);
                    face = new DrawElementsUInt(PrimitiveSet::TRIANGLES, 0);
                    face->push_back(size);
                    face->push_back(size + i);
                    face->push_back(size + i + 1);
                    geom->addPrimitiveSet(face);
                    //tgeom->addPrimitiveSet(face);
                }
            }

            StateSet* state(fgeode->getOrCreateStateSet());
            Material* mat(new Material);
            mxml_node_t* color_node = mxmlFindElement(node, tree, "color", NULL, NULL, MXML_DESCEND);
            double colors[3];
            double colorsl[3];
            mxml_node_t* color_child = color_node->child;

            for (int i = 0; i < 4; i++) //New
            {
                colors[i] = atof(color_child->value.text.string);
                colorsl[i] = atof(color_child->value.text.string);

                if ((colorsl[i] != 0) && (i != 3))
                {
                    colorsl[i] = colorsl[i] - 20;
                }

                if (i != 3)
                {
                    colors[i] = colors[i] / 255;
                    colorsl[i] = colorsl[i] / 255;
                }

                color_child = color_child->next;
            }

            //Vec4f color = Vec4f(colors[0],colors[1],colors[2],0.4);
            Vec4f color = Vec4f(colors[0], colors[1], colors[2], colors[3]);  //Replaced
            Vec4f colorl = Vec4f(colorsl[0], colorsl[1], colorsl[2], colorsl[3]);  //New
            Vec4f colort = Vec4f(colors[0], colors[1], colors[2], colors[3]);  //New
            mat->setColorMode(Material::DIFFUSE);
            mat->setDiffuse(Material::FRONT_AND_BACK, color);
            state->setAttribute(mat);
            state->setRenderingHint(StateSet::TRANSPARENT_BIN);
            state->setMode(GL_BLEND, StateAttribute::ON);
            state->setMode(GL_LIGHTING, StateAttribute::OFF);
            osg::PolygonMode* polymode = new osg::PolygonMode;
            polymode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
            state->setAttributeAndModes(polymode, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
            fgeode->setStateSet(state);
            fgeode->addDrawable(geom);
            StateSet* state2(lgeode->getOrCreateStateSet());
            Material* mat2(new Material);
            state2->setRenderingHint(StateSet::OPAQUE_BIN);
            mat2->setColorMode(Material::DIFFUSE);
            mat2->setDiffuse(Material::FRONT_AND_BACK, colorl);
            state2->setAttribute(mat2);
            state->setMode(GL_BLEND, StateAttribute::ON);
            state2->setMode(GL_LIGHTING, StateAttribute::OFF);
            osg::PolygonMode* polymode2 = new osg::PolygonMode;
            polymode2->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
            state2->setAttributeAndModes(polymode2, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
            lgeode->setStateSet(state2);
            lgeode->addDrawable(geom);
            StateSet* state3(tgeode->getOrCreateStateSet());
            Material* mat3(new Material);
            state3->setRenderingHint(StateSet::OPAQUE_BIN);
            //state3->setRenderingHint(StateSet::TRANSPARENT_BIN);
            mat3->setColorMode(Material::DIFFUSE);
            mat3->setDiffuse(Material::FRONT_AND_BACK, colort);
            state3->setAttribute(mat3);
            state3->setMode(GL_BLEND, StateAttribute::ON);
            state3->setMode(GL_LIGHTING, StateAttribute::OFF);
            osg::PolygonMode* polymode3 = new osg::PolygonMode;
            polymode3->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
            state3->setAttributeAndModes(polymode3, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
            tgeode->setStateSet(state3);
            tgeode->addDrawable(tgeom);
            loc->geom = geom;
            loc->fill_geode = fgeode;
            loc->line_geode = lgeode;
            loc->top_geode = tgeode;
            query->sphereRoot->addChild(loc->fill_geode);
            query->sphereRoot->addChild(loc->line_geode);
            // query->sphereRoot->addChild(loc->top_geode);
            //query->sphereRoot->removeChild(loc->top_geode);
            //setNodeMask(0xffffffff)
            Geode* textGeode = new Geode();
            loc->text_geode = textGeode;
            StateSet* textSS = textGeode->getOrCreateStateSet();
            textSS->setRenderingHint(StateSet::TRANSPARENT_BIN);
            textSS->setMode(GL_BLEND, StateAttribute::ON);
            textSS->setMode(GL_LIGHTING, StateAttribute::OFF);
            loc->label = new osgText::Text();
            loc->label->setText(loc->name);
            loc->label->setAlignment(osgText::Text::CENTER_CENTER);
            textGeode->addDrawable(loc->label);

            for (int i = 0; i < size; i++)
            {
                float width = abs(loc->coordsTop[i].z() - loc->coordsBot[i].z());
                //printf("Width %f\n", width);
                Vec3d edge = (loc->coordsBot[(i + 1) % size] - loc->coordsBot[i]);
                Matrix scale;
                // double scaleFactor = (min(min((float)edge.length()/600,(float)width/60),0.7f))/10;
                double scaleFactor = 0.000200;
                //printf("Scale %f\n", scaleFactor);//0.000167
                scale.makeScale(scaleFactor, scaleFactor, scaleFactor);
                MatrixTransform* scaleTrans = new MatrixTransform();
                scaleTrans->setMatrix(scale);
                Matrix rot1;
                // rot1.makeRotate(acos(edge.x()/edge.length()),Vec3f(0,-edge.z(),edge.y()));
                rot1.makeRotate(acos(edge.x() / edge.length()), Vec3f(0, 0, edge.y()));
                //rot1.makeRotate(acos(edge.x()/edge.length()),Vec3f(edge.y(),0,0));
                //rot1.makeRotate(acos(edge.x()/edge.length()),Vec3f(0.5,edge.z(),edge.y()));
                //rot1.makeRotate(acos(edge.x()/edge.length()),Vec3d(edge.x(),edge.y(),edge.z()));
                //printf("Angle %f %f %f %f\n",acos(edge.x()/edge.length()), edge.x(), edge.y(), edge.z());
                //test
                Matrix rot2;
                rot2.makeRotate(osg::DegreesToRadians(90.0), edge.x(), edge.y(), 0);
                Matrix rot3;
                rot3.makeRotate(osg::DegreesToRadians(180.0), 0, 0, 1);
                MatrixTransform* rotTrans = new MatrixTransform();
                rotTrans->setMatrix(rot1 * rot2 * rot3);
                //rotTrans->setMatrix(rot1);
                Matrix pos;
                Vec3d norm = (loc->coordsBot[i] - loc->coordsTop[i]) ^ (loc->coordsTop[(i + 1) % size] - loc->coordsTop[i]);
                norm /= norm.length();
                norm *= 5;
                //printf("Norm %f %f %f\n",norm[0], norm[1], norm[2]);
                Vec3d posF = ((loc->coordsBot[i] + loc->coordsBot[(i + 1) % size] + loc->coordsTop[i] + loc->coordsTop[(i + 1) % size]) / 4);
                //printf("Face %f %f %f\n",posF[0], posF[1], posF[2]);
                //pos.makeTranslate((loc->coordsBot[i]+loc->coordsBot[(i+1)%size]+loc->coordsTop[i]+loc->coordsTop[(i+1)%size])/4+norm);
                pos.makeTranslate(posF);
                //printf("Position %f %f %f\n",pos[0], pos[1], pos[2]);
                MatrixTransform* posTrans = new MatrixTransform();
                posTrans->setMatrix(pos);
                scaleTrans->addChild(loc->text_geode);
                rotTrans->addChild(scaleTrans);
                posTrans->addChild(rotTrans);
                //posTrans->addChild(scaleTrans);
                query->sphereRoot->addChild(posTrans);
                locCenter += loc->coordsTop[i];
            }

            bool topLabel = true;

            if (topLabel)
            {
                //Vec3d posF = loc->coordsTop[0];
                double tl;
                double old;
                int vertice = 0;

                for (int i = 0; i < loc->coordsTop.size(); i++)
                {
                    tl = ((loc->coordsTop[i].x()) * -1) + loc->coordsTop[i].y();

                    if (i == 0)
                    {
                        old = tl;
                    }
                    else
                    {
                        if (tl > old)
                        {
                            old = tl;
                            vertice = i;
                        }
                    }
                }

                //double edge = loc->coordsTop[0].y() - loc->coordsTop[1].y();
                double y;
                Vec3d posF = loc->coordsTop[vertice];
                int loclength = loc->name.length() / 2;
                double locOffset = loclength * 0.1;
                posF = posF + Vec3d(locOffset, -0.1, 0.03);
                Matrix pos;
                pos.makeTranslate(posF);
                MatrixTransform* posTrans = new MatrixTransform();
                posTrans->setMatrix(pos);
                double scaleFactor = 0.000300;
                Matrix scale;
                scale.makeScale(scaleFactor, scaleFactor, scaleFactor);
                MatrixTransform* scaleTrans = new MatrixTransform();
                scaleTrans->setMatrix(scale);
                scaleTrans->addChild(loc->text_geode);
                posTrans->addChild(scaleTrans);
                query->sphereRoot->addChild(posTrans);
                //cout << pos.x() << " " << pos.x() << " " << pos.x() << " "
            }

            locCenter /= size;
            loc->label->setCharacterSize(300);
        }

        //center+=locCenter;
        query->loci.push_back(loc);
    }

    //center/=query->loci.size();
    // query->center = center;
#ifdef WITH_OSSIMPLANET

    if (_ossim)
        OssimPlanet::instance()->addModel(query->sphereRoot, center.y(), center.x(), Vec3(1, 1, 1), 10, 0, 0, 0);

#endif
    std::cerr << "Loci Loaded." << std::endl;
}
void ArtifactVis2::setupSiteMenu()
{

    _modelDropDown = new MenuButton("3D Models");
    _modelDropDown->setCallback(this);
    modelDropped = false; 
   _infoPanel->addMenuItem(_modelDropDown);

    _pcDropDown = new MenuButton("Point Clouds");
    _pcDropDown->setCallback(this);
    pcDropped = false; 
   _infoPanel->addMenuItem(_pcDropDown);

   // _hudDropDown = new MenuButton("HUD Models");
   // _hudDropDown->setCallback(this);
   // hudDropped = false; 
  // _infoPanel->addMenuItem(_hudDropDown);

/*
    _modelDisplayMenu = new SubMenu("Models");
    _displayMenu->addItem(_modelDisplayMenu);
    _pcDisplayMenu = new SubMenu("Point Clouds");
    _displayMenu->addItem(_pcDisplayMenu);
    _hudDisplayMenu = new SubMenu("HUD");
    _displayMenu->addItem(_hudDisplayMenu);
*/
return;
    cout << "Generating Model menu..." << endl;
    string file = ConfigManager::getEntry("Plugin.ArtifactVis2.3DModelFolder").append("models.kml");   //Replaced
    FILE* fp = fopen(file.c_str(), "r");

    if (fp == NULL)
    {
        std::cerr << "Unable to open file: " << file << std::endl;
        return;
    }

    const double M_TO_MM = 1.0f;
    // const double LATLONG_FACTOR = 100000.0f;
    Vec3d offset = Vec3d(
                       ConfigManager::getFloat("Plugin.ArtifactVis2.Offset.X", 0),
                       ConfigManager::getFloat("Plugin.ArtifactVis2.Offset.Y", 0),
                       ConfigManager::getFloat("Plugin.ArtifactVis2.Offset.Z", 0));
    mxml_node_t* tree;
    tree = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
    fclose(fp);

    if (tree == NULL)
    {
        std::cerr << "Unable to parse XML file: " << file << std::endl;
        return;
    }

    mxml_node_t* node = mxmlFindElement(tree, tree, "Placemark", NULL, NULL, MXML_DESCEND);

    for (; node != NULL; node = mxmlFindElement(node, tree, "Placemark", NULL, NULL, MXML_DESCEND))
    {
        mxml_node_t* child = mxmlFindElement(node, tree, "name", NULL, NULL, MXML_DESCEND);
        string child_text = child->child->value.text.string;
        bool isPC = child_text.find("PointCloud") != string::npos;
        bool isSite = child_text.find("Model") != string::npos;
        bool isHud = child_text.find("HUD") != string::npos;
        if(isSite || isHud) continue;
        child = mxmlFindElement(node, tree, "href", NULL, NULL, MXML_DESCEND);
        MenuCheckbox* site;
	if(isHud)
	{
        site = new MenuCheckbox(child->child->value.text.string, true);
	}
	else
	{
        site = new MenuCheckbox(child->child->value.text.string, false);
	}
        site->setCallback(this);
        MenuButton* reload = new MenuButton("-Reload");
        reload->setCallback(this);
        int factor[1];
        double trans[3];
        double scale[3];
        double rot[3];
        child = mxmlFindElement(node, tree, "altitudeMode", NULL, NULL, MXML_DESCEND);
        factor[0] = atoi(child->child->value.text.string);
        child = mxmlFindElement(node, tree, "longitude", NULL, NULL, MXML_DESCEND);
        trans[0] = atof(child->child->value.text.string);
        child = mxmlFindElement(node, tree, "latitude", NULL, NULL, MXML_DESCEND);
        trans[1] = atof(child->child->value.text.string);
        child = mxmlFindElement(node, tree, "altitude", NULL, NULL, MXML_DESCEND);
        trans[2] = atof(child->child->value.text.string);
        child = mxmlFindElement(node, tree, "x", NULL, NULL, MXML_DESCEND);
        scale[0] = atof(child->child->value.text.string);
        child = mxmlFindElement(node, tree, "y", NULL, NULL, MXML_DESCEND);
        scale[1] = atof(child->child->value.text.string);
        child = mxmlFindElement(node, tree, "z", NULL, NULL, MXML_DESCEND);
        scale[2] = atof(child->child->value.text.string);
        child = mxmlFindElement(node, tree, "tilt", NULL, NULL, MXML_DESCEND);
        rot[0] = atof(child->child->value.text.string);
        child = mxmlFindElement(node, tree, "heading", NULL, NULL, MXML_DESCEND);
        rot[1] = atof(child->child->value.text.string);
        child = mxmlFindElement(node, tree, "roll", NULL, NULL, MXML_DESCEND);
        rot[2] = atof(child->child->value.text.string);
        Vec3d position(trans[0], trans[1], trans[2]);
        Matrixd transMat;
        transMat.makeTranslate(position);
        Matrixd scaleMat;
        //scaleMat.makeScale(M_TO_MM*LATLONG_FACTOR, M_TO_MM*LATLONG_FACTOR, M_TO_MM);
        scaleMat.makeScale(M_TO_MM, M_TO_MM, M_TO_MM);
        Matrixd rot1;
        rot1.makeRotate(osg::DegreesToRadians(-90.0), 0, 1, 0);
        Matrixd rot2;
        rot2.makeRotate(osg::DegreesToRadians(90.0), 1, 0, 0);
        Matrixd mirror;
        mirror.makeScale(1, -1, 1);
        Matrixd offsetMat;
        offsetMat.makeTranslate(offset);

        if (isPC)
        {
            _pcRoot.push_back(new MatrixTransform());
            Vec3d pos = Vec3d(0, 0, 0) * transMat;
           // _infoPanel->addMenuItem(site);
           // _infoPanel->addMenuItem(reload);
            _showPCCB.push_back(site);
            _reloadPC.push_back(reload);
            _pcPos.push_back(pos);
            _pcScale.push_back(Vec3d(scale[0], scale[1], scale[2]));
            _pcRot.push_back(Vec3d(rot[0], rot[1], rot[2]));
            _pcFactor.push_back(factor[0]);
        }
        else if(isSite)
        {
            _siteRoot.push_back(new MatrixTransform());
            Vec3d pos;

            //if(_ossim)
            if (true)
            {
                pos = position;
                //cout << "Ossim position: ";
            }
            else
            {
                //pos = Vec3d(0,0,0) * mirror * transMat * scaleMat * mirror * rot2 * rot1 * offsetMat;
            }

            //cout << pos[0] << ", " << pos[1] << ", " << pos[2] << endl;
           // _infoPanel->addMenuItem(site);
           // _infoPanel->addMenuItem(reload);
           // _showModelCB.push_back(site);
            _reloadModel.push_back(reload);
            _sitePos.push_back(pos);
            _siteScale.push_back(Vec3d(scale[0], scale[1], scale[2]));
            _siteRot.push_back(Vec3d(rot[0], rot[1], rot[2]));
        }
        else if(isHud)
        {
            _hudRoot.push_back(new MatrixTransform());
            Vec3d pos;

                pos = position;
           // _hudDisplayMenu->addItem(site);
           // _hudDisplayMenu->addItem(reload);
            _showHudCB.push_back(site);
            _reloadHud.push_back(reload);
            _hudPos.push_back(pos);
            _hudScale.push_back(Vec3d(scale[0], scale[1], scale[2]));
            _hudRot.push_back(Vec3d(rot[0], rot[1], rot[2]));
           // int curIndex = _hudRoot.size() - 1;
        }
    }

    int countMP = _pcRoot.size() + _siteRoot.size() + _hudRoot.size();
    cout << "Total Models and PC loaded: " << countMP << "\n";
    _coordsPC.resize(countMP);
    _colorsPC.resize(countMP);
    _avgOffset.resize(countMP);
    _modelSFileNode.resize(countMP);
    _hudFileNode.resize(countMP);
    //readHudFile(0);
    //SceneManager::instance()->getScene()->addChild(_hudRoot[0]);
    cout << "done." << endl;
}
void ArtifactVis2::reloadSite(int index)
{
    //string file = ConfigManager::getEntry("Plugin.ArtifactVis2.ArchInterfaceFolder").append("Model/KISterrain2.kml");
    string file = ConfigManager::getEntry("Plugin.ArtifactVis2.3DModelFolder").append("models.kml");   //Replaced
    FILE* fp = fopen(file.c_str(), "r");

    if (fp == NULL)
    {
        std::cerr << "Unable to open file: " << file << std::endl;
        return;
    }

    const double M_TO_MM = 1.0f;
    //const double LATLONG_FACTOR = 100000.0f;
    //const double LATLONG_FACTOR = 1000.0f;
    Vec3d offset = Vec3d(
                       ConfigManager::getFloat("Plugin.ArtifactVis2.Offset.X", 0),
                       ConfigManager::getFloat("Plugin.ArtifactVis2.Offset.Y", 0),
                       ConfigManager::getFloat("Plugin.ArtifactVis2.Offset.Z", 0));
    //bang1
    mxml_node_t* tree;
    tree = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
    fclose(fp);

    if (tree == NULL)
    {
        std::cerr << "Unable to parse XML file: " << file << std::endl;
        return;
    }

    mxml_node_t* node = mxmlFindElement(tree, tree, "Placemark", NULL, NULL, MXML_DESCEND);
    int incPC = 0;
    int incModel = 0;
    int incHud = 0;

    for (; node != NULL; node = mxmlFindElement(node, tree, "Placemark", NULL, NULL, MXML_DESCEND))
    {
        mxml_node_t* child = mxmlFindElement(node, tree, "name", NULL, NULL, MXML_DESCEND);
        string child_text = child->child->value.text.string;
        bool isPC = child_text.find("PointCloud") != string::npos;
        bool isSite = child_text.find("Model") != string::npos;
        bool isHud = child_text.find("HUD") != string::npos;
        child = mxmlFindElement(node, tree, "href", NULL, NULL, MXML_DESCEND);
        //MenuCheckbox * site = new MenuCheckbox(child->child->value.text.string,false);
        //site->setCallback(this);
        //MenuButton * reload = new MenuButton("-Reload");
        //reload->setCallback(this);
        int factor[1];
        double trans[3];
        double scale[3];
        double rot[3];
        child = mxmlFindElement(node, tree, "altitudeMode", NULL, NULL, MXML_DESCEND);
        factor[0] = atoi(child->child->value.text.string);
        child = mxmlFindElement(node, tree, "longitude", NULL, NULL, MXML_DESCEND);
        trans[0] = atof(child->child->value.text.string);
        child = mxmlFindElement(node, tree, "latitude", NULL, NULL, MXML_DESCEND);
        trans[1] = atof(child->child->value.text.string);
        child = mxmlFindElement(node, tree, "altitude", NULL, NULL, MXML_DESCEND);
        trans[2] = atof(child->child->value.text.string);
        child = mxmlFindElement(node, tree, "x", NULL, NULL, MXML_DESCEND);
        scale[0] = atof(child->child->value.text.string);
        child = mxmlFindElement(node, tree, "y", NULL, NULL, MXML_DESCEND);
        scale[1] = atof(child->child->value.text.string);
        child = mxmlFindElement(node, tree, "z", NULL, NULL, MXML_DESCEND);
        scale[2] = atof(child->child->value.text.string);
        child = mxmlFindElement(node, tree, "tilt", NULL, NULL, MXML_DESCEND);
        rot[0] = atof(child->child->value.text.string);
        child = mxmlFindElement(node, tree, "heading", NULL, NULL, MXML_DESCEND);
        rot[1] = atof(child->child->value.text.string);
        child = mxmlFindElement(node, tree, "roll", NULL, NULL, MXML_DESCEND);
        rot[2] = atof(child->child->value.text.string);
        Vec3d position(trans[0], trans[1], trans[2]);
        Matrixd transMat;
        transMat.makeTranslate(position);
        Matrixd scaleMat;
        //scaleMat.makeScale(M_TO_MM*LATLONG_FACTOR, M_TO_MM*LATLONG_FACTOR, M_TO_MM);
        scaleMat.makeScale(M_TO_MM, M_TO_MM, M_TO_MM);
        Matrixd rot1;
        rot1.makeRotate(osg::DegreesToRadians(-90.0), 0, 1, 0);
        Matrixd rot2;
        rot2.makeRotate(osg::DegreesToRadians(90.0), 1, 0, 0);
        Matrixd mirror;
        mirror.makeScale(1, -1, 1);
        Matrixd offsetMat;
        offsetMat.makeTranslate(offset);

        if (isPC)
        {
            if (index == incPC)
            {
                //_pcRoot.push_back(new MatrixTransform());
                Vec3d pos = Vec3d(0, 0, 0) * transMat;
                //_pcDisplayMenu->addItem(site);
                //_pcDisplayMenu->addItem(reload);
                //_showPCCB.push_back(site);
                //_reloadPC.push_back(reload);
                _pcPos[incPC] = pos;
                _pcScale[incPC] = Vec3d(scale[0], scale[1], scale[2]);
                _pcRot[incPC] = Vec3d(rot[0], rot[1], rot[2]);
                _pcFactor[incPC] = factor[0];
            }

            incPC++;
        }
        else if (isSite)
        {
            //_siteRoot.push_back(new MatrixTransform());
            Vec3d pos;

            //if(_ossim)
            if (true)
            {
                pos = position;
                //cout << "Ossim position: ";
            }
            else
            {
                //pos = Vec3d(0,0,0) * mirror * transMat * scaleMat * mirror * rot2 * rot1 * offsetMat;
            }

            if (index == incModel)
            {
                //cout << pos[0] << ", " << pos[1] << ", " << pos[2] << endl;
                //_modelDisplayMenu->addItem(site);
                //_modelDisplayMenu->addItem(reload);
                //_showModelCB.push_back(site);
                //_reloadModel.push_back(reload);
                //cout << "Reset _SiteRoot\n";
                //_siteRoot[incModel] = new MatrixTransform();
                _sitePos[incModel] = pos;
                _siteScale[incModel] = Vec3d(scale[0], scale[1], scale[2]);
                _siteRot[incModel] = Vec3d(rot[0], rot[1], rot[2]);
            }

            incModel++;
        }
	else if(isHud)
	{
            Vec3d pos;

                pos = position;

            if (index == incHud)
            {
                _hudPos[incModel] = pos;
                _hudScale[incModel] = Vec3d(scale[0], scale[1], scale[2]);
                _hudRot[incModel] = Vec3d(rot[0], rot[1], rot[2]);
            }

            incHud++;
	}
    }

    cout << "done." << endl;
}
void ArtifactVis2::setupQuerySelectMenu()
{
    vector<std::string> queryNames;
    vector<bool> queryActive;
    _displayMenu->removeItem(_artifactDisplayMenu);
    _displayMenu->removeItem(_locusDisplayMenu);
    _artifactDisplayMenu = new SubMenu("Artifacts");
    _displayMenu->addItem(_artifactDisplayMenu);
    _locusDisplayMenu = new SubMenu("Loci");
    _displayMenu->addItem(_locusDisplayMenu);

    for (int i = 0; i < _query.size(); i++)
    {
        _root->removeChild(_query[i]->sphereRoot);
        queryNames.push_back(_query[i]->name);
        queryActive.push_back(_query[i]->active);
    }

    _query.clear();
    _locusDisplayMode = new MenuTextButtonSet(true, 300, 30, 1);
    _locusDisplayMode->addButton("Fill");
    _locusDisplayMode->setValue("Fill", true);
    _locusDisplayMode->addButton("Wireframe");
    _locusDisplayMode->addButton("Solid");
    _locusDisplayMode->addButton("Top");
    _locusDisplayMode->setCallback(this);
    _locusDisplayMenu->addItem(_locusDisplayMode);
    _queryOptionMenu.clear();
    _queryOption.clear();
    _showQueryInfo.clear();
    _queryInfo.clear();
    _queryDynamicUpdate.clear();
    _eraseQuery.clear();
    _centerQuery.clear();
    _toggleLabel.clear();
    //std::string file = ConfigManager::getEntry("Plugin.ArtifactVis2.ArchInterfaceFolder").append("kmlfiles.xml");
    std::string file = ConfigManager::getEntry("Plugin.ArtifactVis2.Database").append("kmlfiles.xml");   //Replaced
    FILE* fp = fopen(file.c_str(), "r");

    if (fp == NULL)
    {
        std::cerr << "Unable to open file: " << file << std::endl;
        return;
    }

    mxml_node_t* tree;
    tree = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
    fclose(fp);

    if (tree == NULL)
    {
        std::cerr << "Unable to parse XML file: " << file << std::endl;
        return;
    }

    mxml_node_t* table = mxmlFindElement(tree, tree, "kmlfiles", NULL, NULL, MXML_DESCEND);
int n = 0;
    for (mxml_node_t* child = table-> child; child != NULL; child = child->next)
    {
        std::string kmlName = child->value.text.string;
        //std::string kmlfile = ConfigManager::getEntry("Plugin.ArtifactVis2.ArchInterfaceFolder").append("queries/").append(kmlName).append(".kml");
        std::string kmlfile = ConfigManager::getEntry("Plugin.ArtifactVis2.Database").append(kmlName).append(".kml");    //Replaced
        FILE* fpkml = fopen(kmlfile.c_str(), "r");

        if (fpkml == NULL)
        {
            std::cerr << "Unable to open file: " << kmlfile << std::endl;
            return;
        }

        mxml_node_t* querytree;
        querytree = mxmlLoadFile(NULL, fpkml, MXML_TEXT_CALLBACK);
        fclose(fpkml);

        if (querytree == NULL)
        {
            std::cerr << "Unable to parse XML file: " << kmlfile << std::endl;
            return;
        }

        bool sf = mxmlFindElement(querytree, querytree, "Polyhedron", NULL, NULL, MXML_DESCEND) == NULL;
        QueryGroup* query = new QueryGroup;
        query->kmlPath = kmlfile;
        query->sf = sf;
        query->sphereRoot = new MatrixTransform();
        readQuery(query);
        SubMenu* queryOptionMenu = new SubMenu(query->name);
        bool isActive = false;

        for (int i = 0; i < queryNames.size(); i++)
        {
            if (kmlName == queryNames[i])
            {
                isActive = queryActive[i];
            }
        }
        //Temporarily Turn on Specific Query File for Testing
	if(ConfigManager::getBool("Plugin.ArtifactVis2.QueryTest"))
	{
        	if(kmlName == ConfigManager::getEntry("Plugin.ArtifactVis2.QueryTest.Query"))
        	{
          	isActive = true;
          	cerr << "KmlName is Active: " << kmlName << "\n";
		}
	}
        query->active = isActive;
        _query.push_back(query);
        MenuCheckbox* queryOption = new MenuCheckbox(kmlName, isActive);
        queryOption->setCallback(this);
        SubMenu* showInfo = new SubMenu("Show info");
        stringstream ss;
        ss << "Query: " << query->query << "\n";
        ss << "Size: " << query->artifacts.size() << " Artifacts\n";
        MenuText* info = new MenuText(ss.str(), 1, false, 400);
        showInfo->addItem(info);
        MenuCheckbox* dynamic = new MenuCheckbox("Dynamically Update", false);
        MenuButton* erase = new MenuButton("Delete this Query");
        erase->setCallback(this);
        MenuButton* center = new MenuButton("Center Query");
        center->setCallback(this);
        MenuCheckbox* toglabel = new MenuCheckbox("Labels OnOff", true);
        toglabel->setCallback(this);
        _queryOptionMenu.push_back(queryOptionMenu);
        if(sf)
        {
        _queryOption.push_back(queryOption);
        _querySfIndex.push_back(n);
        _showQueryInfo.push_back(showInfo);
        }
        else
        {
        _queryOptionLoci.push_back(queryOption);
        _showQueryInfoLoci.push_back(showInfo);
        _queryLociIndex.push_back(n);

        }
        _queryInfo.push_back(info);
        _queryDynamicUpdate.push_back(dynamic);
        _eraseQuery.push_back(erase);
        _centerQuery.push_back(center);
        _toggleLabel.push_back(toglabel);
       // queryOptionMenu->addItem(queryOption);
       // queryOptionMenu->addItem(showInfo);
        //_infoPanel->addMenuItem(queryOption);
       // _infoPanel->addMenuItem(showInfo);
       // queryOptionMenu->addItem(dynamic);

        if (kmlName != "query" && kmlName != "querp")
        {
           // queryOptionMenu->addItem(erase);
        }

       // queryOptionMenu->addItem(center);
       // queryOptionMenu->addItem(toglabel);  //new
        _root->addChild(query->sphereRoot);

        if (!isActive) query->sphereRoot->setNodeMask(0);
        else if (query->sf) displayArtifacts(query);

        if (sf)
            _artifactDisplayMenu->addItem(queryOptionMenu);
        else
            _locusDisplayMenu->addItem(queryOptionMenu);
    n++;
    }

    cout << "Menu loaded." << endl;
}
void ArtifactVis2::setupTablesMenu()
{
    std::string file = ConfigManager::getEntry("Plugin.ArtifactVis2.Database").append("tables.xml");   //Replaced Problemo
    FILE* fp = fopen(file.c_str(), "r");

    //FILE * fp = NULL;
    if (fp == NULL)
    {
        // std::cerr << "Unable to open file: " << file << std::endl;
        return;
    }

    mxml_node_t* tree;
    tree = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
    fclose(fp);

    if (tree == NULL)
    {
        std::cerr << "Unable to parse XML file: " << file << std::endl;
        return;
    }

    _qsPanel = new TabbedDialogPanel(100, 30, 4, "QuerySystem", "Plugin.ArtifactVis2.QsPanel");
    _qsPanel->setVisible(false);
    mxml_node_t* table = mxmlFindElement(tree, tree, "tables", NULL, NULL, MXML_DESCEND);

    for (mxml_node_t* child = table-> child; child != NULL; child = child->next)
    {
        std::string tableName = child->value.text.string;
        Table* table = new Table;
        table->name = tableName;
        SubMenu* tableMenu = new SubMenu(tableName);
        table->queryMenu = tableMenu;
        //_tablesMenu->addItem(tableMenu);
        _qsPanel->addMenuItem(tableMenu);
        _tables.push_back(table);
    }

    std::cerr << "Table.xml read!" << std::endl;
}
void ArtifactVis2::setupQueryMenu(Table* table)
{
    bool status;
    std::string file = ConfigManager::getEntry("Plugin.ArtifactVis2.Database").append(table->name).append(".xml");    //New
    FILE* fp = fopen(file.c_str(), "r");

    if (ComController::instance()->isMaster())
    {
        if (fp == NULL)
        {
            chdir(ConfigManager::getEntry("Plugin.ArtifactVis2.ArchInterfaceFolder").c_str());
            stringstream ss;
            ss << "./ArchInterface -m \"" << table->name << "\"";
            system(ss.str().c_str());
        }

        ComController::instance()->sendSlaves(&status, sizeof(bool));
    }
    else
    {
        ComController::instance()->readMaster(&status, sizeof(bool));
    }

    table->query_view = new MenuText("", 1, false, 400);

    //std::string file = ConfigManager::getEntry("Plugin.ArtifactVis2.ArchInterfaceFolder").append("menu.xml");
    //std::string file = ConfigManager::getEntry("Plugin.ArtifactVis2.Database").append("menu.xml"); //Replaced
    if (fp == NULL)
    {
        fp = fopen(file.c_str(), "r");
    }

    if (fp == NULL)
    {
        std::cerr << "Unable to open file: " << file << std::endl;
        return;
    }

    mxml_node_t* tree;
    tree = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
    fclose(fp);

    if (tree == NULL)
    {
        std::cerr << "Unable to parse XML file: " << file << std::endl;
        return;
    }

    table->conditions = new SubMenu("Edit Conditions");
    table->queryMenu->addItem(table->conditions);
    mxml_node_t* node = mxmlFindElement(tree, tree, NULL, NULL, NULL, MXML_DESCEND);

    for (; node != NULL; node = mxmlFindElement(node, tree, NULL, NULL, NULL, MXML_DESCEND))
    {
        string name(node->value.element.name);
        SubMenu* menu = new SubMenu(name);
        mxml_node_t* child;
        int childcount = 0;

        for (child = node->child; child != NULL; child = child->next)
        {
            childcount++;
        }

        if (childcount <= 30)
        {
            MenuTextButtonSet* optionSet = new MenuTextButtonSet(true, 500, 30, 1);
            std::vector<string> children;

            for (child = node->child; child != NULL; child = child->next)
            {
                std::string optionName = child->value.text.string;

                if (optionName.empty()) break;

                children.push_back(optionName);
            }

            sort(children.begin(), children.end());

            for (int i = 0; i < children.size(); i++)
            {
                optionSet->addButton(children[i]);
            }

            optionSet->setCallback(this);
            table->queryOptions.push_back(optionSet);
            menu->addItem(optionSet);
            table->querySubMenu.push_back(menu);
        }
        else
        {
            std::vector<string> children;

            for (child = node->child; child != NULL; child = child->next)
            {
                string childText = child->value.text.string;

                if (childText.empty()) break;

                children.push_back(childText);
            }

            sort(children.begin(), children.end());
            table->sliderEntry.push_back(children);
            MenuList* slider = new MenuList();
            slider->setValues(children);
            slider->setCallback(this);
            table->queryOptionsSlider.push_back(slider);
            MenuCheckbox* useSlider = new MenuCheckbox("Use Value", false);
            useSlider->setCallback(this);
            table->querySlider.push_back(useSlider);
            menu->addItem(useSlider);
            menu->addItem(slider);
            table->querySubMenuSlider.push_back(menu);
        }

        table->conditions->addItem(menu);
    }

    table->addOR = new MenuButton("Add OR Condition");
    table->addOR->setCallback(this);
    table->queryMenu->addItem(table->addOR);
    table->removeOR = new MenuButton("Remove Last OR Condition");
    table->removeOR->setCallback(this);
    table->queryMenu->addItem(table->removeOR);
    table->viewQuery = new SubMenu("View Current Query");
    table->viewQuery->addItem(table->query_view);
    table->queryMenu->addItem(table->viewQuery);
    table->genQuery = new MenuButton("Generate Query");
    table->genQuery->setCallback(this);
    table->queryMenu->addItem(table->genQuery);
    table->clearConditions = new MenuButton("Clear All Conditions");
    table->clearConditions->setCallback(this);
    table->queryMenu->addItem(table->clearConditions);
    table->saveQuery = new MenuButton("Save Current Query");
    table->saveQuery->setCallback(this);
    table->queryMenu->addItem(table->saveQuery);
   // _tablesMenu->addItem(table->queryMenu);
  // _qsPanel->addMenuItem(table->queryMenu);
}
void ArtifactVis2::setupFlyToMenu()
{
    std::string file = ConfigManager::getEntry("Plugin.ArtifactVis2.Database").append("flyto.xml");
    FILE* fp = fopen(file.c_str(), "r");

    //FILE * fp = NULL;
    if (fp == NULL)
    {
        std::cerr << "Unable to open file: " << file << std::endl;
        return;
    }

    mxml_node_t* tree;
    tree = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
    fclose(fp);

    if (tree == NULL)
    {
        std::cerr << "Unable to parse XML file: " << file << std::endl;
        return;
    }

    _bookmarkPanel = new TabbedDialogPanel(100, 30, 4, "Bookmarks", "Plugin.ArtifactVis2.BookmarkPanel");
    _bookmarkPanel->setVisible(false);
  //  _flyMenu = new SubMenu("Fly To");
  //  _avMenu->addItem(_flyMenu);
    mxml_node_t* fly_node = mxmlFindElement(tree, tree, "flyto", NULL, NULL, MXML_DESCEND);
    mxml_node_t* fly_child;
    _flyplace = new FlyPlace;

    for (fly_child = fly_node->child; fly_child != NULL; fly_child = fly_child->next)
    {
        mxml_node_t* type_child;
        std::string flyname = mxmlFindElement(fly_child, tree, "name", NULL, NULL, MXML_DESCEND)->child->value.text.string;
        //cerr << "Fly: " << flyname << "\n";
        MenuButton* gotoP = new MenuButton(flyname);  //new
        gotoP->setCallback(this);
        _bookmarkPanel->addMenuItem(gotoP);
        _goto.push_back(gotoP);
        double scale;
        //double x;
        //double y;
        //double z;
        _flyplace->name.push_back(flyname);
        type_child = mxmlFindElement(fly_child, tree, "scale", NULL, NULL, MXML_DESCEND)->child;
        scale = atof(type_child->value.text.string);
        _flyplace->scale.push_back(scale);
        type_child = mxmlFindElement(fly_child, tree, "x", NULL, NULL, MXML_DESCEND)->child;
        _flyplace->x.push_back((atof(type_child->value.text.string) *scale));
        type_child = mxmlFindElement(fly_child, tree, "y", NULL, NULL, MXML_DESCEND)->child;
        _flyplace->y.push_back((atof(type_child->value.text.string) *scale));
        type_child = mxmlFindElement(fly_child, tree, "z", NULL, NULL, MXML_DESCEND)->child;
        _flyplace->z.push_back((atof(type_child->value.text.string) *scale));
        type_child = mxmlFindElement(fly_child, tree, "rx", NULL, NULL, MXML_DESCEND)->child;
        _flyplace->rx.push_back(atof(type_child->value.text.string));
        type_child = mxmlFindElement(fly_child, tree, "ry", NULL, NULL, MXML_DESCEND)->child;
        _flyplace->ry.push_back(atof(type_child->value.text.string));
        type_child = mxmlFindElement(fly_child, tree, "rz", NULL, NULL, MXML_DESCEND)->child;
        _flyplace->rz.push_back(atof(type_child->value.text.string));
        type_child = mxmlFindElement(fly_child, tree, "rw", NULL, NULL, MXML_DESCEND)->child;
        _flyplace->rw.push_back(atof(type_child->value.text.string));
        //cerr << "Scale: " << _flyplace->scale[0] << "\n";
    }

    std::cerr << "flyto.xml read!" << std::endl;
}
void ArtifactVis2::updateSelect()
{
    osg::Vec3 markPos(0, 1000, 0);
    markPos = markPos * PluginHelper::getHandMat();
    osg::Matrix markTrans;
    markTrans.makeTranslate(markPos);
    _selectMark->setMatrix(markTrans);

    if (_selectActive)
    {
        osg::Matrix w2l = PluginHelper::getWorldToObjectTransform();
        _selectCurrent = osg::Vec3(0, 1000, 0);
        _selectCurrent = _selectCurrent * PluginHelper::getHandMat() * w2l;
    }

    if (_selectStart.length2() > 0)
    {
        osg::BoundingBox bb;
        osg::Vec3 minvec, maxvec;
        minvec.x() = std::min(_selectStart.x(), _selectCurrent.x());
        minvec.y() = std::min(_selectStart.y(), _selectCurrent.y());
        minvec.z() = std::min(_selectStart.z(), _selectCurrent.z());
        maxvec.x() = std::max(_selectStart.x(), _selectCurrent.x());
        maxvec.y() = std::max(_selectStart.y(), _selectCurrent.y());
        maxvec.z() = std::max(_selectStart.z(), _selectCurrent.z());
        bb.set(minvec, maxvec);
        osg::Matrix scale, trans;
        trans.makeTranslate(bb.center());
        scale.makeScale(maxvec.x() - minvec.x(), maxvec.y() - minvec.y(), maxvec.z() - minvec.z());
        _selectBox->setMatrix(scale * trans);
        std::map<string, int> dcCount;
        int totalSelected = 0;

        for (int q = 0; q < _query.size(); q++)
        {
            vector<Artifact*> artifacts = _query[q]->artifacts;

            if (_query[q]->active)
                for (int i = 0; i < artifacts.size(); i++)
                {
                    if (bb.contains(artifacts[i]->modelPos) && !artifacts[i]->selected)
                    {
                        osg::ShapeDrawable* sd = dynamic_cast<osg::ShapeDrawable*>(artifacts[i]->drawable);

                        if (sd)
                        {
                            osg::Vec4 color = sd->getColor();
                            color.x() = color.x() * 2.0;
                            color.y() = color.y() * 2.0;
                            color.z() = color.z() * 2.0;
                            sd->setColor(color);
                        }

                        artifacts[i]->selected = true;
                    }
                    else if ((!artifacts[i]->visible || !bb.contains(artifacts[i]->modelPos)) && artifacts[i]->selected)
                    {
                        osg::ShapeDrawable* sd = dynamic_cast<osg::ShapeDrawable*>(artifacts[i]->drawable);

                        if (sd)
                        {
                            osg::Vec4 color = sd->getColor();
                            color.x() = color.x() * 0.5;
                            color.y() = color.y() * 0.5;
                            color.z() = color.z() * 0.5;
                            sd->setColor(color);
                        }

                        artifacts[i]->selected = false;
                    }

                    if (artifacts[i]->selected)
                    {
                        dcCount[artifacts[i]->dc]++;
                        totalSelected++;
                    }
                }
        }

        std::stringstream ss;
        ss << "Region Size: " << fabs(_selectStart.x() - _selectCurrent.x()) << " x " << fabs(_selectStart.y() - _selectCurrent.y()) << " x " << fabs(_selectStart.z() - _selectCurrent.z()) << std::endl;
        ss << "Artifacts Selected: " << totalSelected;

        for (std::map<std::string, int>::iterator it = dcCount.begin(); it != dcCount.end(); it++)
        {
            ss << std::endl << it->first << ": " << it->second;
        }

        _selectionStatsPanel->setText(ss.str());
    }
}

std::vector<Vec3> ArtifactVis2::getArtifactsPos()
{
    vector<Vec3f> positions;

    for (int q = 0; q < _query.size(); q++)
    {
        vector<Artifact*>::iterator item = _query[q]->artifacts.begin();

        for (int i = 0; item != _query[q]->artifacts.end(); item++)
        {
            positions.push_back((*item)->modelPos);
            i++;
        }
    }

    return positions;
}
float ArtifactVis2::selectArtifactSelected()
{
    //float transMult = SpaceNavigator::instance()->transMultF();
    return transMult;
}
void ArtifactVis2::testSelected()
{
    cout << _activeArtifact;
}
osg::Matrix ArtifactVis2::getSelectMatrix()
{
    return _selectModelLoad.get()->getMatrix();
}
void ArtifactVis2::setSelectMatrix(osg::Matrix& mat)
{
    _selectModelLoad.get()->setMatrix(mat);
}
void ArtifactVis2::rotateModel(double rx, double ry, double rz)
{
    //cout << rx << "," << ry << "," << rz << "\n";
    //cout << _selectRotx << "," << _selectRoty << "," << _selectRotz << "\n";
    if (_handOn)
    {
        rx *= 10;
        ry *= 10;
        rz *= 10;
        _selectRotx += rx;
        _selectRoty += ry;
        _selectRotz += rz;
    }
    else
    {
        _selectRotx += rx;
        _selectRoty += ry;
        _selectRotz += rz;
    }

    //cout << _selectRotx << "," << _selectRoty << "," << _selectRotz << "\n";
    _root->removeChild(_selectModelLoad.get());
    Matrixd scale;
    scale.makeScale(_snum, _snum, _snum);
    MatrixTransform* scaleTrans = new MatrixTransform();
    scaleTrans->setMatrix(scale);
    scaleTrans->addChild(_modelFileNode);
    MatrixTransform* siteRote = new MatrixTransform();   //Andrew here it works, with no problem
    Matrixd rotx;
    rotx.makeRotate(osg::DegreesToRadians(_selectRotx), 1, 0, 0);   //Rotating it 10 degrees
    Matrixd roty;
    roty.makeRotate(osg::DegreesToRadians(_selectRoty), 0, 1, 0);   //Rotating it 10 degrees
    Matrixd rotz;
    rotz.makeRotate(osg::DegreesToRadians(_selectRotz), 0, 1, 0);   //Rotating it 10 degrees
    siteRote->setMatrix(rotx * roty * rotz);
    siteRote->addChild(scaleTrans);
    PositionAttitudeTransform* modelTrans = new PositionAttitudeTransform();
    modelTrans->setPosition(_modelartPos);
    modelTrans->addChild(siteRote);
    _selectModelLoad = new osg::MatrixTransform();
    _selectModelLoad->addChild(modelTrans);
    _root->addChild(_selectModelLoad.get());  //reattach the transform matrix.
}



void ArtifactVis2::moveCam(double bscale, double x, double y, double z, double o1, double o2, double o3, double o4)
{
    Vec3 trans = Vec3(x, y, z) * bscale;
    Matrix tmat;
    tmat.makeTranslate(trans);
    Matrix rot;
    rot.makeRotate(osg::Quat(o1, o2, o3, o4));
    Matrixd gotoMat = rot * tmat;
    //Matrixd gotoMat = tmat; //Since rotation is the same for site we need no rotation
    Matrixd camMat = PluginHelper::getObjectMatrix();
    float cscale = PluginHelper::getObjectScale();
    Vec3 camTrans = camMat.getTrans();
    Quat camQuad = camMat.getRotate();
    //cout << (camTrans.x() / cscale) << "," << (camTrans.y() / cscale) << "," << (camTrans.z() / cscale) << " Scale:" << cscale << " Rot:" << camQuad.x() << "," << camQuad.y() << "," << camQuad.z() << "\n";
    PluginHelper::setObjectMatrix(gotoMat);
    PluginHelper::setObjectScale(bscale);
}


void ArtifactVis2::createSelObj(osg::Vec3 pos, string color, float radius)
{
/*
    if (_modelFileNode == NULL)
        _modelFileNode = osgDB::readNodeFile(ConfigManager::getEntry("Plugin.ArtifactVis2.ScanFolder").append("/50563/50563.ply"));

    if (_modelFileNode2 == NULL)
        _modelFileNode2 = osgDB::readNodeFile(ConfigManager::getEntry("Plugin.ArtifactVis2.3DModelFolder").append("/photos/50035/frame.obj"));

    Matrixd scale;
    double snum = 0.002 * m2mm;

    if (color == "DD") snum *= 10;

    scale.makeScale(snum, snum, snum);
    MatrixTransform* modelScaleTrans = new MatrixTransform();
    modelScaleTrans->setMatrix(scale);

    if (color == "DD") modelScaleTrans->addChild(_modelFileNode2);
    else modelScaleTrans->addChild(_modelFileNode);

    Vec3d poz0(0, 0, 0);  //(0.15,-0.15,1.1);
    Box* sphereShape = new Box(poz0, radius);
    ShapeDrawable* ggg2 = new ShapeDrawable(sphereShape);
    ggg2->setColor(_colors[ArtifactVis2::dc2Int(color)]);
    osg::Geode* boxGeode = new osg::Geode;
    boxGeode->addDrawable(ggg2);
    MatrixTransform* rotate = new osg::MatrixTransform();
    Matrix rotMat;
    rotMat.makeRotate(0, 1, 0, 1);
    rotate->setMatrix(rotMat);
    rotate->addChild(boxGeode);
    MatrixTransform* translate = new osg::MatrixTransform();
    osg::Matrixd tmat;
    tmat.makeTranslate(pos);
    translate->setMatrix(tmat);
    translate->addChild(rotate);
    _root->addChild(translate);
    selectableItems.push_back(SelectableItem(boxGeode, modelScaleTrans, translate, rotate, snum));
*/
}

void ArtifactVis2::flyTo(int i)
{

            double x, y, z, rx, ry, rz, rw;
            x = y = z = rx = ry = rz = rw = 0.0;
            double bscale;
	if(_flyplace->scale.size() >= i)
	{	
            bscale = _flyplace->scale[i];
            //x=-2231700;
            //y=-4090410;
            //z=-81120.3;
            x = _flyplace->x[i]/bscale;
            y = _flyplace->y[i]/bscale;
            z = _flyplace->z[i]/bscale;
            rx = _flyplace->rx[i];
            ry = _flyplace->ry[i];
            rz = _flyplace->rz[i];
            rw = _flyplace->rw[i];

            moveCam(bscale,x,y,z,rx,ry,rz,rw);
	}
}

void ArtifactVis2::updateHudMovement(int i, TrackedButtonInteractionEvent * tie,float _moveDistance, osg::Vec3 _menuPoint)
{
    osg::Vec3 menuPoint = osg::Vec3(0,_moveDistance,0);
    //std::cerr << "move dist: " << _moveDistance << std::endl;
    menuPoint = menuPoint * tie->getTransform();

    //TODO: add hand/head mapping
    osg::Vec3 viewerPoint =
            TrackingManager::instance()->getHeadMat(0).getTrans();

    osg::Vec3 viewerDir = viewerPoint - menuPoint;
    viewerDir.z() = 0.0;

    osg::Matrix menuRot;
    menuRot.makeRotate(osg::Vec3(0,-1,0),viewerDir);

    _hudRoot[i]->setMatrix(
            osg::Matrix::translate(-_menuPoint) * menuRot
                    * osg::Matrix::translate(menuPoint));
cerr << "Finished \n";
}
/*
void ArtifactVis2::updateSceneObjectMovement(DSIntersector::DSIntersector* mDSIntersector, cvr::TrackedButtonInteractionEvent * tie)
{
                osg::Vec3 hit = mDSIntersector->getWorldHitPosition();
                osg::Vec3 orig = _hudRoot[0]->getMatrix().getTrans();
                rhit = hit - orig;
                rhit.y() *= -1;
                //rhit.z() -= 10;
                osg::Vec3 normal = mDSIntersector->getWorldHitNormal();

                osg::Node *hitCAVEGeode = mDSIntersector->getHitNode();
              //  cerr << "Pointer:" << pointerPos.x()<< " " << pointerPos.y()<< " " << pointerPos.z();
               osg::Vec3 geodeCenter = hitCAVEGeode->getBound().center();
                cerr << "\nHit:" << hit.x()<< " " << hit.y()<< " " << hit.z() << "\n";
                cerr << "\nRHit:" << rhit.x()<< " " << rhit.y()<< " " << rhit.z() << "\n";
              //  cerr << "\nNormal:" << normal.x()<< " " << normal.y()<< " " << normal.z();
                cerr << "\nGeodeCenter:" << geodeCenter.x()<< " " << geodeCenter.y()<< " " << geodeCenter.z();
		if(hitCAVEGeode->asGeode())
		{
             //  osg::Transform* test  =  hitCAVEGeode->asTransform(); 
		//	cerr << "Is Geode\n";
		}
                if(hitCAVEGeode == _hudRoot[0])
		{
		//	cerr << " Same";
		}
                osg::Vec3 oldPos = _hudRoot[0]->getMatrix().getTrans();
//	cerr << oldPos.x() << " " << oldPos.y() << " "<< oldPos.z() << "\n";
//printf("%g %g %g",oldPos.x(),oldPos.y(),oldPos.z());

                    std::map<int,osg::Vec3> _currentPoint;
		osg::Vec3 ray = _currentPoint[tie->getHand()]
                            - tie->getTransform().getTrans();

                    float _moveDistance;
                    osg::Vec3 _menuPoint;
                    _moveDistance = ray.length();
		  // cerr << "Move: " <<_moveDistance << "\n";
                    _menuPoint = _currentPoint[tie->getHand()]
                            * osg::Matrix::inverse(_hudRoot[0]->getMatrix());
                   
		  //   updateHudMovement(0,tie,_moveDistance,_menuPoint);

                if(!_hudActive)
		{
                 _hudActive = true;
                  grabCurrentPos = hit;
                  prevHandRot = TrackingManager::instance()->getHandMat(0).getRotate();
		}
                else
		{
                  osg::Matrix updateMatrix;
                  osg::Vec3 _ray = grabCurrentPos - hit;
                  osg::Vec3f _dist = _ray;
                  if(_dist.x() != 0 || _dist.y() != 0 || _dist.z() != 0)
		 {
                //  cerr << "Move: " << _dist.x() << " " << _dist.y() << " " << _dist.z();
		 }
                 bool useMouse;
                 useMouse = false;
                 if(useMouse)
		 {
                   _dist.y() = 0;
                 }
                 // updateMatrix = _hudRoot[0]->getMatrix();
	          updateMatrix.makeTranslate(_dist);
                  osg::Matrix inverseMatrix = osg::Matrix::inverse(updateMatrix);

                   if(tie->getButton() == 1)
		   {
                      cerr << "Button 2 Down\n";
		   }
                osg::Quat handRot = TrackingManager::instance()->getHandMat(0).getRotate();
                osg::Quat distRot =  handRot - prevHandRot;
                prevHandRot = handRot;
  		//  cerr << "rx: " << handRot.x() << " ry : " << handRot.y() << " rz : " << handRot.z() << " rw : " << handRot.w()  << "\n"; 
    		Matrix tmat;
                //rhit = hit;
    		osg::Vec3 center = hit;

    		cerr << "x: " << center.x() << " y: " << center.y() << " z: " << center.z() << "\n"; 
    		tmat.makeTranslate(Vec3(0,0,0));
        	Matrix rot;
        	//rot.makeRotate(rx, xa, ry, ya, rz, za);
        	rot.makeRotate(distRot);
        	Matrixd gotoMat = osg::Matrix::translate(-center)  * rot * osg::Matrix::translate(center);
                osg::Matrix inverseRotMatrix = osg::Matrix::inverse(gotoMat);
		

                  _hudRoot[0]->setMatrix(_hudRoot[0]->getMatrix() * inverseMatrix );
                  grabCurrentPos = hit;
		  
		  _grabActive = true;
        	}
	     

}
*/
void ArtifactVis2::loadAnnotationGraph(int inc)
{
osg::Vec3 pos;
 //   Vec4f color = Vec4f(0, (204/255), (204/255), 1);
   // Vec4f color = Vec4f(1, 1, 1, 1);
    Vec4f colorl = Vec4f(0, 0, 1, 0.4);
    float r = 0;
    float g = 107/255;
    float b = 235/255;
    Vec4f color = Vec4f(0, 0.42, 0.92, 1);

//Create Quad Face
float width = 300;
float height = 500;
pos = Vec3(-(width/2),0,-(height/2));

//0 127 255 0.4
    osg::Geometry * geo = new osg::Geometry();
    osg::Vec3Array* verts = new osg::Vec3Array();
  //  verts->push_back(pos);
  //  verts->push_back(pos + osg::Vec3(width,0,0));
  //  verts->push_back(pos + osg::Vec3(width,0,height));
  //  verts->push_back(pos + osg::Vec3(0,0,height));
   // pos -= Vec3(,0,0);
    //pos -= Vec3(0,0,250);
    verts->push_back(pos);
    verts->push_back(pos + osg::Vec3(width,0,0));
    verts->push_back(pos + osg::Vec3(width,0,height));
    verts->push_back(pos + osg::Vec3(0,0,height));

    geo->setVertexArray(verts);

    osg::DrawElementsUInt * ele = new osg::DrawElementsUInt(
            osg::PrimitiveSet::QUADS,0);

    ele->push_back(0);
    ele->push_back(1);
    ele->push_back(2);
    ele->push_back(3);
    geo->addPrimitiveSet(ele);


    Geode* fgeode = new Geode();
    StateSet* state(fgeode->getOrCreateStateSet());
    Material* mat(new Material);

            mat->setColorMode(Material::DIFFUSE);
            mat->setDiffuse(Material::FRONT_AND_BACK, color);
            state->setAttribute(mat);
            state->setRenderingHint(StateSet::TRANSPARENT_BIN);
            state->setMode(GL_BLEND, StateAttribute::ON);
            state->setMode(GL_LIGHTING, StateAttribute::OFF);
            osg::PolygonMode* polymode = new osg::PolygonMode;
            polymode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
            state->setAttributeAndModes(polymode, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
            fgeode->setStateSet(state);
  
            _annotations[inc]->geo = geo;
            fgeode->addDrawable(_annotations[inc]->geo);

//Line Geode

    Geode* lgeode = new Geode();
            StateSet* state2(lgeode->getOrCreateStateSet());
            Material* mat2(new Material);
            state2->setRenderingHint(StateSet::OPAQUE_BIN);
            mat2->setColorMode(Material::DIFFUSE);
            mat2->setDiffuse(Material::FRONT_AND_BACK, color);
            state2->setAttribute(mat2);
            state->setMode(GL_BLEND, StateAttribute::ON);
            state2->setMode(GL_LIGHTING, StateAttribute::OFF);

            osg::LineWidth* linewidth1 = new osg::LineWidth();
            linewidth1->setWidth(2.0f); 
            state2->setAttribute(linewidth1);

            osg::PolygonMode* polymode2 = new osg::PolygonMode;
            polymode2->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
            state2->setAttributeAndModes(polymode2, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
            lgeode->setStateSet(state2);
            lgeode->addDrawable(_annotations[inc]->geo);

cerr << "Pass\n";

//Text Geode
   Geode* textGeode = new Geode();
    float size = 25;
   //std::string text = "Hello World this is just a test of the textbox wrap feature, which is Awesome"; 
   std::string text = _annotations[inc]->desc; 

 osgText::Text* textNode  = new osgText::Text();
    textNode->setCharacterSize(size);
    textNode->setAlignment(osgText::Text::LEFT_TOP);
    Vec3 tPos = pos + osg::Vec3(5,-5,(height-5));
   // Vec3 tPos = pos;
    textNode->setPosition(tPos);
    textNode->setColor(color);
   // textNode->setBackdropColor(osg::Vec4(0,0,0,0));
    textNode->setAxisAlignment(osgText::Text::XZ_PLANE);
    textNode->setText(text);
    textNode->setMaximumWidth(width);
    textNode->setFont(CalVR::instance()->getHomeDir() + "/resources/arial.ttf");

    textGeode->addDrawable(textNode);

    _annotations[inc]->textNode = textNode;

    string name = _annotations[inc]->name;







	    SceneObject * so;
	    so = new SceneObject(name, false, false, false, true, false);
	   // so = new SceneObject(name, false, false, false, false, false);
	    osg::Switch* switchNode = new osg::Switch();
	    so->addChild(switchNode);
	    PluginHelper::registerSceneObject(so,"Test");
	    so->attachToScene();
//Add geode to switchNode
//	switchNode->addChild(fgeode);
	switchNode->addChild(lgeode);
	switchNode->addChild(textGeode);

	    so->setNavigationOn(true);
if(!_annotations[inc]->fromFile)
{
	    so->setMovable(true);
}
	    so->addMoveMenuItem();
	    so->addNavigationMenuItem();

	    SubMenu * sm = new SubMenu("Position");
	    so->addMenuItem(sm);

	    MenuButton * mb;
	    mb = new MenuButton("Load");
	    mb->setCallback(this);
	    sm->addItem(mb);
	    //_loadMap[so] = mb;

	    SubMenu * savemenu = new SubMenu("Save");
	    sm->addItem(savemenu);
	    //_saveMenuMap[so] = savemenu;

	    mb = new MenuButton("Save");
	    mb->setCallback(this);
	    savemenu->addItem(mb);
            std::map<cvr::SceneObject*,cvr::MenuButton*> _saveMap;
	  //  _saveMap[so] = mb;
            _annotations[inc]->saveMap = mb;
	    mb = new MenuButton("Reset");
	    mb->setCallback(this);
	    sm->addItem(mb);
	    //_resetMap[so] = mb;

	    mb = new MenuButton("Delete");
	    mb->setCallback(this);
	    so->addMenuItem(mb);
	    //_deleteMap[so] = mb;
            _annotations[inc]->deleteMap = mb;
            MenuCheckbox * mc;
	    mc = new MenuCheckbox("Active",_annotations[inc]->active);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            _annotations[inc]->activeMap = mc;

	    mc = new MenuCheckbox("Visible",_annotations[inc]->visible);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            _annotations[inc]->visibleMap = mc;

if(_annotations[inc]->fromFile)
{
                so->setPosition(_annotations[inc]->pos);
                so->setRotation(_annotations[inc]->rot);
               // so->setScale(_annotations[inc]->scale);
}
else
{

                so->setPosition(_annotations[inc]->pos);

}

    _annotations[inc]->so = so;
    _annotations[inc]->pos = _annotations[inc]->so->getPosition();
    _annotations[inc]->rot = _annotations[inc]->so->getRotation();
if(!_annotations[inc]->fromFile)
{
    _annotations[inc]->lStart = _annotations[inc]->so->getPosition();
    _annotations[inc]->lEnd = _annotations[inc]->so->getPosition();
}
  //  _annotations[inc]->lEnd += Vec3(1,1,1);
//Vec3 posl = _annotations[inc]->lStart;
//                cerr << "x: " << posl.x() << " y: " << posl.y() << " z: " << posl.z() << std::endl;
//posl = _annotations[inc]->pos;
  //              cerr << "x: " << posl.x() << " y: " << posl.y() << " z: " << posl.z() << std::endl;

   // _annotations[inc]->connector = new osg::Geometry();
    osg:Geometry* connector = new osg::Geometry();
    verts = new osg::Vec3Array();
    verts->push_back(_annotations[inc]->lStart);
    verts->push_back(_annotations[inc]->lEnd);

  //  _annotations[inc]->connector->setVertexArray(verts);
    connector->setVertexArray(verts);

    ele = new osg::DrawElementsUInt(
            osg::PrimitiveSet::LINES,0);

    ele->push_back(0);
    ele->push_back(1);
  //  _annotations[inc]->connector->addPrimitiveSet(ele);
   connector->addPrimitiveSet(ele);

    osg::Vec4Array* colors = new osg::Vec4Array;
    colors->push_back(color);

    osg::TemplateIndexArray<unsigned int,osg::Array::UIntArrayType,4,4> *colorIndexArray;
    colorIndexArray = new osg::TemplateIndexArray<unsigned int,
            osg::Array::UIntArrayType,4,4>;
    colorIndexArray->push_back(0);
    colorIndexArray->push_back(0);
   
  //  _annotations[inc]->connector->setColorArray(colors);
   connector->setColorArray(colors);
//    _annotations[inc]->connector->setColorIndices(colorIndexArray);
  // connector->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
   connector->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    Geode* connectorGeode = new Geode();

            StateSet* state3(connectorGeode->getOrCreateStateSet());
           // Material* mat3(new Material);
           // state3->setRenderingHint(StateSet::OPAQUE_BIN);
           // mat2->setColorMode(Material::DIFFUSE);
          //  mat2->setDiffuse(Material::FRONT_AND_BACK, colorl);
           // state2->setAttribute(mat2);
           // state->setMode(GL_BLEND, StateAttribute::ON);
         //   state3->setMode(GL_LIGHTING, StateAttribute::OFF);
          //  osg::PolygonMode* polymode2 = new osg::PolygonMode;

            osg::LineWidth* linewidth = new osg::LineWidth();
            linewidth->setWidth(2.0f); 
          //  polymode2->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
            state3->setAttributeAndModes(linewidth, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
            connectorGeode->setStateSet(state3);




    _annotations[inc]->connector = connector;
    connectorGeode->addDrawable(_annotations[inc]->connector);
     _annotations[inc]->connectorGeode = connectorGeode;
    
    Group* connectorNode = new Group();
    _annotations[inc]->connectorNode = connectorNode;
    _annotations[inc]->connectorNode->addChild(_annotations[inc]->connectorGeode);
    _root->addChild(_annotations[inc]->connectorNode);






}
std::string ArtifactVis2::getCharacterAscii(int code)
{

string character = "";
std::string ascii[] = {"a", "b", "c", "d", "e", "f", "g" , "h" , "i" , "j" , "k" , "l" , "m" , "n" , "o" , "p" , "q" , "r" , "s" , "t" , "u" , "v" , "w" , "x" , "y" , "z" };

if(code > 96 && code < 123)
{
 //Is LowerCase letters
 int n = 0;
 for (int i = 97; i < 123; i++)
 {
   if(code == i)
   {
      character = ascii[n];
      break;
   }
   n++;
 }
}
else if(code > 64 && code < 91)
{
 //Is LowerCase letters
 int n = 0;
 for (int i = 65; i < 91; i++)
 {
   if(code == i)
   {
      ascii[n][0] = toupper(ascii[n][0]);
      character = ascii[n];
      
      break;
   }
   n++;
 }
}
else if(code > 39 && code < 65)
{
 std::string number[] = {"(",")","*","+",",","-",".","/","0","1","2","3","4","5","6","7","8","9",":",";","<","=",">","?","@"};
 int n = 0;

 for (int i = 40; i < 65; i++)
 {
   if(code == i)
   {
      character = number[n];
      
      break;
   }
   n++;
 }

}
else if(code == 32)
{
 character = " ";
}


return character;
}

void ArtifactVis2::readAnnotationFile()
{

    cout << "Reading Annotation File..." << endl;
string filename = ConfigManager::getEntry("Plugin.ArtifactVis2.Database").append("annotation.kml");
    FILE* fp;
    mxml_node_t* tree;
    fp = fopen(filename.c_str(), "r");

    if (fp == NULL)
    {
        std::cerr << "Unable to open file: " << filename << std::endl;
        return;
    }

    tree = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
    fclose(fp);

    if (tree == NULL)
    {
        std::cerr << "Unable to parse XML file: "  << std::endl;
        return;
    }

    mxml_node_t* node = mxmlFindElement(tree, tree, "Placemark", NULL, NULL, MXML_DESCEND);
    int inc = 0;
    for (; node != NULL; node = mxmlFindElement(node, tree, "Placemark", NULL, NULL, MXML_DESCEND))
    {

        deactivateAllAnno();
       //Create Annotation Struc
       Annotation* anno = new Annotation;

        mxml_node_t* desc_node = mxmlFindElement(node, tree, "description", NULL, NULL, MXML_DESCEND);
        mxml_node_t* desc_child;
//        desc_child = desc_node->child;
  //          char* desc_text = desc_child->value.text.string;
            string desc = "";


        for (desc_child = desc_node->child; desc_child != NULL; desc_child = desc_child->next)
        {
            string desc_text = desc_child->value.text.string;
            desc.append(desc_text);
            desc.append(" ");

           // desc_child = desc_child->next;
        }











        anno->desc = desc;
//Name
        desc_node = mxmlFindElement(node, tree, "name", NULL, NULL, MXML_DESCEND);
        desc_child = desc_node->child;
        char*    desc_text = desc_child->value.text.string;
            string name = desc_text;
        anno->name = name;
//Type
        desc_node = mxmlFindElement(node, tree, "type", NULL, NULL, MXML_DESCEND);
        desc_child = desc_node->child;
            desc_text = desc_child->value.text.string;
            string type = desc_text;
        anno->type = type;
//Graph Position and Orientation
  string var;
  float pos[3];
  float rot[4];

        desc_node = mxmlFindElement(node, tree, "longitude", NULL, NULL, MXML_DESCEND);
        desc_child = desc_node->child;
            var = desc_child->value.text.string;
            pos[0] = atof(var.c_str());           

        desc_node = mxmlFindElement(node, tree, "latitude", NULL, NULL, MXML_DESCEND);
        desc_child = desc_node->child;
            var = desc_child->value.text.string;
            pos[1] = atof(var.c_str());           
        
        desc_node = mxmlFindElement(node, tree, "altitude", NULL, NULL, MXML_DESCEND);
        desc_child = desc_node->child;
            var = desc_child->value.text.string;
            pos[2] = atof(var.c_str());           

        desc_node = mxmlFindElement(node, tree, "range", NULL, NULL, MXML_DESCEND);
        desc_child = desc_node->child;
            var = desc_child->value.text.string;
            rot[0] = atof(var.c_str());           

        desc_node = mxmlFindElement(node, tree, "tilt", NULL, NULL, MXML_DESCEND);
        desc_child = desc_node->child;
            var = desc_child->value.text.string;
            rot[1] = atof(var.c_str());           

        desc_node = mxmlFindElement(node, tree, "heading", NULL, NULL, MXML_DESCEND);
        desc_child = desc_node->child;
            var = desc_child->value.text.string;
            rot[2] = atof(var.c_str());           

        desc_node = mxmlFindElement(node, tree, "w", NULL, NULL, MXML_DESCEND);
        desc_child = desc_node->child;
            var = desc_child->value.text.string;
            rot[3] = atof(var.c_str());          

  osg::Vec3 coord = Vec3(pos[0],pos[1],pos[2]); 
  osg::Quat quad = Quat(rot[0],rot[1],rot[2],rot[3]); 

        anno->pos = coord;
        anno->rot = quad;
//Line Coords

            mxml_node_t* line_node = mxmlFindElement(node, tree, "lineStart", NULL, NULL, MXML_DESCEND);
            float posStart[3];
            mxml_node_t* line_child = line_node->child;

            for (int i = 0; i < 3; i++)
            {
                posStart[i] = atof(line_child->value.text.string);

                line_child = line_child->next;
            }


            line_node = mxmlFindElement(node, tree, "lineEnd", NULL, NULL, MXML_DESCEND);
            float posEnd[3];
            line_child = line_node->child;

            for (int i = 0; i < 3; i++)
            {
                posEnd[i] = atof(line_child->value.text.string);

                line_child = line_child->next;
            }

  osg::Vec3 lStart = Vec3(posStart[0],posStart[1],posStart[2]); 
  osg::Vec3 lEnd = Vec3(posEnd[0],posEnd[1],posEnd[2]); 
        anno->lStart = lStart;
        anno->lEnd = lEnd;

   anno->active = false;
   anno->visible = true;
   anno->scale = 0.001;
   anno->fromFile = true;


  _annotations.push_back(anno);
   loadAnnotationGraph(inc);
   inc++;
  }
}
void ArtifactVis2::createAnnotationFile(osg::Matrix tie)
{
cerr << "Creating New Annotation\n";
/*
            Matrix handMat;

        osg::Matrixd w2o = PluginHelper::getWorldToObjectTransform();

        Vec3 handTrans = osg::Vec3(0, 0, 0) * TrackingManager::instance()->getHandMat(0) * w2o;
          //  osg::Matrix w2l = PluginHelper::getWorldToObjectTransform();
          //  handMat = PluginHelper::getHandMat(0) * w2l;
            //handMat = tie * w2l;
          //  Vec3 handTrans = handMat.getTrans();
          //  Quat handQuad = handMat.getRotate();i
          Quat handQuad = Vec4(0,0,0,1);
            cerr << "Hand Output: " << handTrans.x() << "," << handTrans.y() << "," << handTrans.z() << " Rot:" << handQuad.x() << "," << handQuad.y() << "," << handQuad.z() << "\n";
*/

//Turn off any Active Annotations
deactivateAllAnno();
//Create new Annotation
           Matrix handMat0 = TrackingManager::instance()->getHandMat(0);
        osg::Matrixd w2o = PluginHelper::getWorldToObjectTransform();
            Matrix handMat;
          Vec3 handTrans = handMat0.getTrans();
          Quat handQuad = handMat0.getRotate();
           // cerr << "Hand Output: " << handTrans.x() << "," << handTrans.y() << "," << handTrans.z() << " Rot:" << handQuad.x() << "," << handQuad.y() << "," << handQuad.z() << "\n";
     //     handTrans = tie.getTrans();
       //   handQuad = tie.getRotate();
         //   cerr << "Hand Output: " << handTrans.x() << "," << handTrans.y() << "," << handTrans.z() << " Rot:" << handQuad.x() << "," << handQuad.y() << "," << handQuad.z() << "\n";
                if(true)
                {
                   // SceneManager::instance()->getMenuRoot()->addChild(       _menuRoot);

                   float   _distance = ConfigManager::getFloat("distance", "MenuSystem.BoardMenu.Position",2000.0);
                    osg::Vec3 menuPoint = osg::Vec3(0,2000,0);
                    menuPoint = menuPoint * handMat0;

                   // if(event->asMouseEvent())
                    if(false)
                    {
                        osg::Vec3 menuOffset = osg::Vec3(0,0,0);
                        osg::Matrix m;
                        m.makeTranslate(menuPoint);
                        handMat = m * w2o;
                    }
                    else
                    {
                        osg::Vec3 viewerPoint =
                                TrackingManager::instance()->getHeadMat(0).getTrans();

                        osg::Vec3 viewerDir = viewerPoint - menuPoint;
                        viewerDir.z() = 0.0;

                        osg::Matrix menuRot;
                        menuRot.makeRotate(osg::Vec3(0,-1,0),viewerDir);

                        osg::Vec3 menuOffset = osg::Vec3(0,0,0);
                        handMat = (osg::Matrix::translate(-menuOffset) * menuRot * osg::Matrix::translate(menuPoint)) * w2o;
                    }

                    //_menuActive = true;
                   // SceneManager::instance()->closeOpenObjectMenu();
                   // return true;
                }


          handTrans = handMat.getTrans();
          handQuad = handMat.getRotate();
osg::Vec3 ball = osg::Vec3(0,0,0);
            cerr << "Hand Output: " << handTrans.x() << "," << handTrans.y() << "," << handTrans.z() << " Rot:" << handQuad.x() << "," << handQuad.y() << "," << handQuad.z() << "\n";





            Annotation* anno = new Annotation;


int count = _annotations.size();
std::stringstream ss;
ss << count;
string name = ss.str();

        anno->name = name;
        string desc = "Comment";
        desc.append(name);
        desc.append(": ");
        anno->desc = desc;
        anno->type = "Basic";
        anno->active = true;
        anno->fromFile = false;

        anno->pos = handTrans;
        anno->rot = handQuad;

  osg::Vec3 lStart = Vec3(0,0,0); 
  osg::Vec3 lEnd = Vec3(0,0,0); 
        anno->lStart = lStart;
        anno->lEnd = lEnd;

   anno->scale = 0.001;


  _annotations.push_back(anno);
cerr << "Count: " << count << "\n";
   loadAnnotationGraph(count);





}
void ArtifactVis2::deactivateAllAnno()
{
    for (int i = 0; i < _annotations.size(); i++)
    {
        _annotations[i]->active = false;
        _annotations[i]->so->setMovable(false);
        _annotations[i]->activeMap->setValue(false);
    }
  
}
void ArtifactVis2::deactivateAllArtifactAnno()
{
    for (int i = 0; i < _artifactAnnoTrack.size(); i++)
    {
        _artifactAnnoTrack[i]->active = false;
       int q = _artifactAnnoTrack[i]->q;
       int art = _artifactAnnoTrack[i]->art;
        _query[q]->artifacts[art]->annotation->so->setMovable(false);
        _query[q]->artifacts[art]->annotation->activeMap->setValue(false);
    }
  
}
void ArtifactVis2::deactivateAllArtifactModel()
{
    for (int i = 0; i < _artifactModelTrack.size(); i++)
    {
        _artifactModelTrack[i]->active = false;
       int q = _artifactModelTrack[i]->q;
       int art = _artifactModelTrack[i]->art;
        _query[q]->artifacts[art]->model->so->setMovable(false);
        _query[q]->artifacts[art]->model->activeMap->setValue(false);
    }
  
}
int ArtifactVis2::findActiveAnnot()
{
    int i;
    bool  found = false;
    for (i = 0; i < _annotations.size(); i++)
    {
       if( _annotations[i]->active) 
       {
         found = true;
         break;
       }
    }
    if(!found)
    {
      i = -1;
    }
    
return i;
}
void ArtifactVis2::updateAnnoLine()
{
  int i;
  if(_annotations.size() > 0)
  {
    for (i = 0; i < _annotations.size(); i++)
    {
    if(_annotations[i]->active)
    {
       Vec3 pos = _annotations[i]->so->getPosition();
       if(_annotations[i]->pos != pos)
       {
 	 _annotations[i]->pos = pos;
         _annotations[i]->lEnd = pos;
         //Update Line
         //...
                osg::Vec3Array* verts = new osg::Vec3Array();
                //_annotations[i]->lEnd = pos;
    		verts->push_back(_annotations[i]->lStart);
    		verts->push_back(_annotations[i]->lEnd);

                 _annotations[i]->connector->setVertexArray(verts);
       }
    break;
    }
    }
 }
}
void ArtifactVis2::updateArtifactLine()
{
  int i;
  if(_artifactAnnoTrack.size() > 0)
  {
    for (i = 0; i < _artifactAnnoTrack.size(); i++)
    {
    if(_artifactAnnoTrack[i]->active)
    {
//_query[q]->artifacts[art]->annotation->
       int q = _artifactAnnoTrack[i]->q;
       int art = _artifactAnnoTrack[i]->art;
       Vec3 pos = _query[q]->artifacts[art]->annotation->so->getPosition();
       if(_query[q]->artifacts[art]->annotation->pos != pos)
       {
 	 _query[q]->artifacts[art]->annotation->pos = pos;
         _query[q]->artifacts[art]->annotation->lEnd = pos;
         //Update Line
         //...
                osg::Vec3Array* verts = new osg::Vec3Array();
                //_annotations[i]->lEnd = pos;
    		verts->push_back(_query[q]->artifacts[art]->annotation->lStart);
    		verts->push_back(_query[q]->artifacts[art]->annotation->lEnd);

                 _query[q]->artifacts[art]->annotation->connector->setVertexArray(verts);
       }
    
    }
    }
 }
}
void ArtifactVis2::updateArtifactModel()
{
  int i;
  if(_artifactModelTrack.size() > 0)
  {
    for (i = 0; i < _artifactModelTrack.size(); i++)
    {
    if(_artifactModelTrack[i]->active)
    {
       int q = _artifactModelTrack[i]->q;
       int art = _artifactModelTrack[i]->art;
       Vec3 pos = _query[q]->artifacts[art]->model->so->getPosition();
       if(_query[q]->artifacts[art]->model->pos != pos)
       {
         Vec3 oldPos = _query[q]->artifacts[art]->model->pos;
         Vec3 distance = pos - oldPos;
 	 _query[q]->artifacts[art]->model->pos = pos;
         _query[q]->artifacts[art]->annotation->lStart = pos;
         _query[q]->artifacts[art]->annotation->lEnd += distance;
 
         //Update Line
         //...
                osg::Vec3Array* verts = new osg::Vec3Array();
                //_annotations[i]->lEnd = pos;
    		verts->push_back(_query[q]->artifacts[art]->annotation->lStart);
    		verts->push_back(_query[q]->artifacts[art]->annotation->lEnd);

                 _query[q]->artifacts[art]->annotation->connector->setVertexArray(verts);

                 _query[q]->artifacts[art]->annotation->pos += distance;
                 _query[q]->artifacts[art]->annotation->so->setPosition(_query[q]->artifacts[art]->annotation->pos);
       }
    
    }
    }
 }
}
void ArtifactVis2::resetArtifactModelOrig(int q, int art)
{
       Vec3 pos = _query[q]->artifacts[art]->model->so->getPosition();
       Vec3 orig = _query[q]->artifacts[art]->model->orig;
       Quat rot = Quat(0,0,0,1);
       if(orig != pos)
       {
       _query[q]->artifacts[art]->model->so->setPosition(orig);
       if(_query[q]->artifacts[art]->model->currentModelType == "frame")
       {
       rot = _query[q]->artifacts[art]->model->frameRot;
       }




       _query[q]->artifacts[art]->model->so->setRotation(rot);

       }
/*
         cerr << "Resetting\n";
       Vec3 pos = _query[q]->artifacts[art]->model->so->getPosition();
       Vec3 orig = _query[q]->artifacts[art]->model->orig;
       if(orig != pos)
       {
         cerr << "Resetting\n";
         Vec3 distance = orig - pos;
         _query[q]->artifacts[art]->annotation->lEnd += distance;
         _query[q]->artifacts[art]->annotation->pos += distance;
 
 	 _query[q]->artifacts[art]->model->pos = orig;
         _query[q]->artifacts[art]->annotation->lStart = orig;
         //Update Line
         //...
                osg::Vec3Array* verts = new osg::Vec3Array();
                //_annotations[i]->lEnd = pos;
    		verts->push_back(_query[q]->artifacts[art]->annotation->lStart);
    		verts->push_back(_query[q]->artifacts[art]->annotation->lEnd);

                 _query[q]->artifacts[art]->annotation->connector->setVertexArray(verts);
                 _query[q]->artifacts[art]->annotation->so->setPosition(_query[q]->artifacts[art]->annotation->pos);
       }
*/
}
                
void ArtifactVis2::saveAnnotationGraph()
{

    mxml_node_t *xml;    /* <?xml ... ?> */
    mxml_node_t *kml;   /* <kml> */
    mxml_node_t *document;   /* <Document> */
    mxml_node_t *name;   /* <name> */
    mxml_node_t *type;   /* <type> */
    mxml_node_t *query;   /* <query> */
    mxml_node_t *timestamp;   /* <timestamp> */

    mxml_node_t *placemark;   /* <Placemark> */
    mxml_node_t *description;   /* <description> */

    mxml_node_t *lookat;   /* <LookAt> */
    mxml_node_t *longitude;   /* <data> */
    mxml_node_t *latitude;   /* <data> */
    mxml_node_t *altitude;   /* <data> */
    mxml_node_t *range;   /* <data> */
    mxml_node_t *tilt;   /* <data> */
    mxml_node_t *heading;   /* <data> */
    mxml_node_t *w;   /* <data> */
    mxml_node_t *styleurl;   /* <data> */
    //mxml_node_t *point;   /* <data> */
    mxml_node_t *altitudeMode;   /* <data> */
    mxml_node_t *coordinates;   /* <data> */
    mxml_node_t *polygon;
    mxml_node_t *extrude;
    mxml_node_t *tessellate;
    mxml_node_t *outerBoundaryIs;
    mxml_node_t *LinearRing;
    mxml_node_t *line;
    mxml_node_t *lineStart;
    mxml_node_t *lineEnd;
    mxml_node_t *color;

//Create KML Container

//KML Name
    string q_name = "annotations";
   // string g_timestamp = getTimeStamp();
    string g_timestamp = "00";

   const char* kml_name = q_name.c_str();
   const char* kml_timestamp = g_timestamp.c_str();

xml = mxmlNewXML("1.0");
        kml = mxmlNewElement(xml, "kml");
            document = mxmlNewElement(kml, "Document");
                name = mxmlNewElement(document, "name");
                  mxmlNewText(name, 0, kml_name);
                timestamp = mxmlNewElement(document, "timestamp");
                  mxmlNewText(timestamp, 0, kml_timestamp);
//.................................................................
//Get Placemarks
int rows = _annotations.size();





//Create Placemarks
for(int m=0; m<rows; m++)
{
 if(!_annotations[m]->deleted)
 {
Vec3 pos = _annotations[m]->so->getPosition();
Quat rot = _annotations[m]->so->getRotation();
float scale = _annotations[m]->so->getScale();
cerr << "Scale" << scale << "\n";
Vec3 flStart = _annotations[m]->lStart;
Vec3 flEnd = _annotations[m]->lEnd;
   //Get Comments Description
   string q_description = _annotations[m]->desc;

stringstream buffer;
buffer << m;
   string e_name = buffer.str();
   buffer.str("");
   buffer << pos.x();
   string q_coordinates = "0 0 0";
   string q_longitude = buffer.str();
   buffer.str("");
   buffer << pos.y();
   string q_latitude = buffer.str();
   buffer.str("");
   buffer << pos.z();
   string q_altitude = buffer.str();
   buffer.str("");
   buffer << rot.x();
   string q_range = buffer.str();
   buffer.str("");
   buffer << rot.y();
   string q_tilt = buffer.str();
   buffer.str("");
   buffer << rot.z();
   string q_heading = buffer.str();
   buffer.str("");
   buffer << rot.w();
   string q_w = buffer.str();
   buffer.str("");
   buffer << flStart.x() << " " <<  flStart.y() << " " << flStart.z();
   string lStart = buffer.str();
   buffer.str("");
   buffer << flEnd.x() << " " <<  flEnd.y() << " " << flEnd.z();
   string lEnd = buffer.str();
   string q_type = "Basic";

                placemark = mxmlNewElement(document, "Placemark");
                    name = mxmlNewElement(placemark, "name");
                      mxmlNewText(name, 0, e_name.c_str());
                    type = mxmlNewElement(placemark, "type");
                      mxmlNewText(type, 0, q_type.c_str());

                    description = mxmlNewElement(placemark, "description");
                      mxmlNewText(description, 0, q_description.c_str());
                    
                    lookat = mxmlNewElement(placemark, "LookAt");
                        longitude = mxmlNewElement(lookat, "longitude");
                          mxmlNewText(longitude, 0, q_longitude.c_str());
                        latitude = mxmlNewElement(lookat, "latitude");
                          mxmlNewText(latitude, 0, q_latitude.c_str());
                        altitude = mxmlNewElement(lookat, "altitude");
                          mxmlNewText(altitude, 0, q_altitude.c_str());
                        range = mxmlNewElement(lookat, "range");
                          mxmlNewText(range, 0, q_range.c_str());
                        tilt = mxmlNewElement(lookat, "tilt");
                          mxmlNewText(tilt, 0, q_tilt.c_str());
                        heading = mxmlNewElement(lookat, "heading");
                          mxmlNewText(heading, 0, q_heading.c_str());
                        w = mxmlNewElement(lookat, "w");
                          mxmlNewText(w, 0, q_w.c_str());
                    
                    styleurl = mxmlNewElement(placemark, "styleUrl");
                      mxmlNewText(styleurl, 0, "#msn_GR");
                    polygon = mxmlNewElement(placemark, "Polygon");
                        extrude = mxmlNewElement(polygon, "extrude");
                          mxmlNewText(extrude, 0, "0");
                        tessellate = mxmlNewElement(polygon, "tessellate");
                          mxmlNewText(tessellate, 0, "1");
                        altitudeMode = mxmlNewElement(polygon, "altitudeMode");
                          mxmlNewText(altitudeMode, 0, "absolute");
                        outerBoundaryIs = mxmlNewElement(polygon, "outerBoundaryIs");
                            LinearRing = mxmlNewElement(outerBoundaryIs, "LinearRing");
                                coordinates = mxmlNewElement(LinearRing, "coordinates");
                                  mxmlNewText(coordinates, 0, q_coordinates.c_str());
                    line = mxmlNewElement(placemark, "Line");
                        lineStart = mxmlNewElement(line, "lineStart");
                          mxmlNewText(lineStart, 0, lStart.c_str());
                        lineEnd = mxmlNewElement(line, "lineEnd");
                          mxmlNewText(lineEnd, 0, lEnd.c_str());
 }
}
//.......................................................
//Save File

  const char *ptr;
    ptr = "";
  ptr = mxmlSaveAllocString(xml, MXML_NO_CALLBACK);
    //cout << ptr;
    FILE *fp;
    
    string filename = ConfigManager::getEntry("Plugin.ArtifactVis2.Database").append("annotation.kml");
    
    kml_name = filename.c_str();
    fp = fopen(kml_name, "w");

    fprintf(fp, ptr);

    fclose(fp);
 

}
/*
mxml_node_t ArtifactVis2::getTree(string filename)
{
    FILE * fp;
  mxml_node_t * tree;
  fp = fopen(filename.c_str(),"r");
  if(fp == NULL){
    std::cerr << "Unable to open file: " << filename << std::endl;
    //return;
  }
  fclose(fp);


  //tree = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);


ifstream in(filename.c_str());
stringstream buffer;
buffer << in.rdbuf();
string contents(buffer.str());

  tree = mxmlLoadString(NULL, contents.c_str(), MXML_NO_CALLBACK);
  //char *ptr = mxmlSaveAllocString(xml, MXML_TEXT_CALLBACK);
  //cout << "LoadingFile\n";
  //fclose(fp);
  if(tree == NULL){
    std::cerr << "Unable to parse XML file: " << filename << std::endl;
    //return;
  }
    return tree;
}
*/
void ArtifactVis2::createArtifactPanel(int q, int art, string desc)
{
//deactivateAllArtifactAnno();
cerr << "Triggered\n";
if(artifactPanelExists(q, art))
{
   cerr << "Exists\n";
   if(!_query[q]->artifacts[art]->annotation->visible)
   {
   cerr << "Not Visible\n";
   _query[q]->artifacts[art]->annotation->so->attachToScene();
   _query[q]->artifacts[art]->annotation->connectorNode->setNodeMask(0xffffffff);
//recreateConnector(q, art);
  // _root->addChild(_query[q]->artifacts[art]->annotation->connectorGeode);
   _query[q]->artifacts[art]->annotation->visible = true;
_query[q]->artifacts[art]->annotation->visibleMap->setValue(true);
                 activateArtifactFromQuery(q, art);
   }
}
else
{
osg::Vec3 pos;
    Vec4f colorl = Vec4f(0, 0, 1, 0.4);
    Vec4f color = Vec4f(0, 0.42, 0.92, 1);
int inc = art;
//Create Quad Face
float width = 300;
float height = 500;
pos = Vec3(-(width/2),0,-(height/2));

    osg::Geometry * geo = new osg::Geometry();
    osg::Vec3Array* verts = new osg::Vec3Array();
    verts->push_back(pos);
    verts->push_back(pos + osg::Vec3(width,0,0));
    verts->push_back(pos + osg::Vec3(width,0,height));
    verts->push_back(pos + osg::Vec3(0,0,height));

    geo->setVertexArray(verts);

    osg::DrawElementsUInt * ele = new osg::DrawElementsUInt(
            osg::PrimitiveSet::QUADS,0);

    ele->push_back(0);
    ele->push_back(1);
    ele->push_back(2);
    ele->push_back(3);
    geo->addPrimitiveSet(ele);


    Geode* fgeode = new Geode();
    StateSet* state(fgeode->getOrCreateStateSet());
    Material* mat(new Material);

            mat->setColorMode(Material::DIFFUSE);
            mat->setDiffuse(Material::FRONT_AND_BACK, color);
            state->setAttribute(mat);
            state->setRenderingHint(StateSet::TRANSPARENT_BIN);
            state->setMode(GL_BLEND, StateAttribute::ON);
            state->setMode(GL_LIGHTING, StateAttribute::OFF);
            osg::PolygonMode* polymode = new osg::PolygonMode;
            polymode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
            state->setAttributeAndModes(polymode, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
            fgeode->setStateSet(state);
  
//             _annotations[inc]->geo = geo;
            fgeode->addDrawable(geo);

//Line Geode

    Geode* lgeode = new Geode();
            StateSet* state2(lgeode->getOrCreateStateSet());
            Material* mat2(new Material);
            state2->setRenderingHint(StateSet::OPAQUE_BIN);
            mat2->setColorMode(Material::DIFFUSE);
            mat2->setDiffuse(Material::FRONT_AND_BACK, color);
            state2->setAttribute(mat2);
            state->setMode(GL_BLEND, StateAttribute::ON);
            state2->setMode(GL_LIGHTING, StateAttribute::OFF);

            osg::LineWidth* linewidth1 = new osg::LineWidth();
            linewidth1->setWidth(2.0f); 
            state2->setAttribute(linewidth1);

            osg::PolygonMode* polymode2 = new osg::PolygonMode;
            polymode2->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
            state2->setAttributeAndModes(polymode2, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
            lgeode->setStateSet(state2);
            lgeode->addDrawable(geo);

cerr << "Pass\n";

//Text Geode
   Geode* textGeode = new Geode();
    float size = 25;
   //std::string text = "Hello World this is just a test of the textbox wrap feature, which is Awesome"; 
cerr << "Query: " << q << " Artifact: " << inc << " Desc: " << desc << "\n";
_query[q]->artifacts[inc]->annotation = new Annotation; 
    _query[q]->artifacts[inc]->annotation->desc = desc;
   std::string text = _query[q]->artifacts[inc]->annotation->desc; 

 osgText::Text* textNode  = new osgText::Text();
    textNode->setCharacterSize(size);
    textNode->setAlignment(osgText::Text::LEFT_TOP);
    Vec3 tPos = pos + osg::Vec3(5,-5,(height-5));
   // Vec3 tPos = pos;
    textNode->setPosition(tPos);
    textNode->setColor(color);
   // textNode->setBackdropColor(osg::Vec4(0,0,0,0));
    textNode->setAxisAlignment(osgText::Text::XZ_PLANE);
    textNode->setText(text);
    textNode->setMaximumWidth(width);
    textNode->setFont(CalVR::instance()->getHomeDir() + "/resources/arial.ttf");

    textGeode->addDrawable(textNode);

   // _annotations[inc]->textNode = textNode;
     _query[q]->artifacts[inc]->annotation->name = _query[q]->artifacts[inc]->values[1];
    string name = _query[q]->artifacts[inc]->annotation->name;







	    SceneObject * so;
	    so = new SceneObject(name, false, false, false, true, false);
	   // so = new SceneObject(name, false, false, false, false, false);
	    osg::Switch* switchNode = new osg::Switch();
	    so->addChild(switchNode);
	    PluginHelper::registerSceneObject(so,"Test");
	    so->attachToScene();
//Add geode to switchNode
//	switchNode->addChild(fgeode);
	switchNode->addChild(lgeode);
	switchNode->addChild(textGeode);

	    so->setNavigationOn(true);
	    so->setMovable(true);
	    so->addMoveMenuItem();
	    so->addNavigationMenuItem();

//cerr << "Pass1\n";
	    SubMenu * sm = new SubMenu("Position");
	    so->addMenuItem(sm);

	    MenuButton * mb;
	    mb = new MenuButton("Load");
	    mb->setCallback(this);
	    sm->addItem(mb);
	    //_loadMap[so] = mb;

	    SubMenu * savemenu = new SubMenu("Save");
	    sm->addItem(savemenu);
	    //_saveMenuMap[so] = savemenu;

	    mb = new MenuButton("Save");
	    mb->setCallback(this);
	    savemenu->addItem(mb);
         //   std::map<cvr::SceneObject*,cvr::MenuButton*> _saveMap;
	  //  _saveMap[so] = mb;
            _query[q]->artifacts[inc]->annotation->saveMap = mb;
	    mb = new MenuButton("Reset");
	    mb->setCallback(this);
	    sm->addItem(mb);
	    //_resetMap[so] = mb;

	    mb = new MenuButton("Delete");
	    mb->setCallback(this);
	    so->addMenuItem(mb);
	    //_deleteMap[so] = mb;

            MenuCheckbox * mc;
	    mc = new MenuCheckbox("Active",true);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            _query[q]->artifacts[inc]->annotation->activeMap = mc;

            
	    mc = new MenuCheckbox("Visible",true);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            _query[q]->artifacts[inc]->annotation->visibleMap = mc;
            _query[q]->artifacts[inc]->annotation->visible = true;
	//	double opos[3];
	//	opos = _query[q]->artifacts[inc]->pos[0];
//		Vec3 orig = Vec3(_query[q]->artifacts[inc]->pos[0],_query[q]->artifacts[inc]->pos[1],_query[q]->artifacts[inc]->pos[2]);
Vec3 orig = _query[q]->artifacts[inc]->modelPos; 
cerr << "Pos: " << orig.x() << " " << orig.y() << " " << orig.z() << "\n";
cerr << "Pass\n";
		_query[q]->artifacts[inc]->annotation->pos = orig;
     
    so->setPosition(_query[q]->artifacts[inc]->annotation->pos);



    _query[q]->artifacts[inc]->annotation->so = so;
    _query[q]->artifacts[inc]->annotation->pos = so->getPosition();
    _query[q]->artifacts[inc]->annotation->rot = so->getRotation();
    _query[q]->artifacts[inc]->annotation->lStart = so->getPosition();
    _query[q]->artifacts[inc]->annotation->lEnd = so->getPosition();

cerr << "Pass\n";
    osg:Geometry* connector = new osg::Geometry();
    verts = new osg::Vec3Array();
    verts->push_back(_query[q]->artifacts[inc]->annotation->lStart);
    verts->push_back(_query[q]->artifacts[inc]->annotation->lEnd);

    connector->setVertexArray(verts);

    ele = new osg::DrawElementsUInt(
            osg::PrimitiveSet::LINES,0);

    ele->push_back(0);
    ele->push_back(1);
   connector->addPrimitiveSet(ele);

    osg::Vec4Array* colors = new osg::Vec4Array;
    colors->push_back(color);

    osg::TemplateIndexArray<unsigned int,osg::Array::UIntArrayType,4,4> *colorIndexArray;
    colorIndexArray = new osg::TemplateIndexArray<unsigned int,
            osg::Array::UIntArrayType,4,4>;
    colorIndexArray->push_back(0);
    colorIndexArray->push_back(0);
   
   connector->setColorArray(colors);
   connector->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    Geode* connectorGeode = new Geode();

            StateSet* state3(connectorGeode->getOrCreateStateSet());
            osg::LineWidth* linewidth = new osg::LineWidth();
            linewidth->setWidth(2.0f); 
            state3->setAttributeAndModes(linewidth, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
            connectorGeode->setStateSet(state3);




    _query[q]->artifacts[inc]->annotation->connector = connector;
    connectorGeode->addDrawable(_query[q]->artifacts[inc]->annotation->connector);

     _query[q]->artifacts[inc]->annotation->connectorGeode = connectorGeode;
    Group* connectorNode = new Group();
    _query[q]->artifacts[inc]->annotation->connectorNode = connectorNode;
    _query[q]->artifacts[inc]->annotation->connectorNode->addChild(_query[q]->artifacts[inc]->annotation->connectorGeode);
    _root->addChild(_query[q]->artifacts[inc]->annotation->connectorNode);

    ArtifactAnnoTrack* artifactAnnoTrack = new ArtifactAnnoTrack;
    artifactAnnoTrack->active = true;
     artifactAnnoTrack->q = q; 
     artifactAnnoTrack->art = inc; 
    _artifactAnnoTrack.push_back(artifactAnnoTrack);
}





}
bool ArtifactVis2::artifactPanelExists(int q, int art)
{
bool exists = false;

    for (int i = 0; i < _artifactAnnoTrack.size(); i++)
    {
        if(_artifactAnnoTrack[i]->q == q && _artifactAnnoTrack[i]->art == art)
        {
          exists = true;
          break;
        } 
    }

return exists;
}
bool ArtifactVis2::artifactModelExists(int q, int art)
{
bool exists = false;

    for (int i = 0; i < _artifactModelTrack.size(); i++)
    {
        if(_artifactModelTrack[i]->q == q && _artifactModelTrack[i]->art == art)
        {
          exists = true;
          break;
        } 
    }

return exists;
}
void ArtifactVis2::activateArtifactFromQuery(int q, int art)
{

    for (int i = 0; i < _artifactAnnoTrack.size(); i++)
    {
        if(_artifactAnnoTrack[i]->q == q && _artifactAnnoTrack[i]->art == art)
        { 
                 _query[q]->artifacts[art]->annotation->active = true;
                 _query[q]->artifacts[art]->annotation->so->setMovable(true);
                 _query[q]->artifacts[art]->annotation->activeMap->setValue(true);
                 _artifactAnnoTrack[i]->active = true;
                 break;
        }
    }

}
void ArtifactVis2::activateModelFromQuery(int q, int art)
{

    for (int i = 0; i < _artifactModelTrack.size(); i++)
    {
        if(_artifactModelTrack[i]->q == q && _artifactModelTrack[i]->art == art)
        { 
                 _query[q]->artifacts[art]->model->active = true;
                 _query[q]->artifacts[art]->model->so->setMovable(true);
                 _query[q]->artifacts[art]->model->activeMap->setValue(true);
                 _artifactModelTrack[i]->active = true;
                 break;
        }
    }

}
void ArtifactVis2::recreateConnector(int q, int art)
{
int inc = art;
    Vec4f color = Vec4f(0, 0.42, 0.92, 1);
    osg:Geometry* connector = new osg::Geometry();
osg::Vec3Array*    verts = new osg::Vec3Array();
    verts->push_back(_query[q]->artifacts[inc]->annotation->lStart);
    verts->push_back(_query[q]->artifacts[inc]->annotation->lEnd);

    connector->setVertexArray(verts);

 osg::DrawElementsUInt *   ele = new osg::DrawElementsUInt(
            osg::PrimitiveSet::LINES,0);

    ele->push_back(0);
    ele->push_back(1);
   connector->addPrimitiveSet(ele);

    osg::Vec4Array* colors = new osg::Vec4Array;
    colors->push_back(color);

    osg::TemplateIndexArray<unsigned int,osg::Array::UIntArrayType,4,4> *colorIndexArray;
    colorIndexArray = new osg::TemplateIndexArray<unsigned int,
            osg::Array::UIntArrayType,4,4>;
    colorIndexArray->push_back(0);
    colorIndexArray->push_back(0);
   
   connector->setColorArray(colors);
   connector->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    Geode* connectorGeode = new Geode();

            StateSet* state3(connectorGeode->getOrCreateStateSet());
            osg::LineWidth* linewidth = new osg::LineWidth();
            linewidth->setWidth(2.0f); 
            state3->setAttributeAndModes(linewidth, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
            connectorGeode->setStateSet(state3);




    _query[q]->artifacts[inc]->annotation->connector = connector;
    connectorGeode->addDrawable(_query[q]->artifacts[inc]->annotation->connector);

     _query[q]->artifacts[inc]->annotation->connectorGeode = connectorGeode;
    _root->addChild(_query[q]->artifacts[inc]->annotation->connectorGeode);

}
 
void ArtifactVis2::createArtifactModel(int q, int art, string desc)
{
cerr << "Triggered\n";
if(artifactModelExists(q, art))
{
   cerr << "Exists\n";
   if(!_query[q]->artifacts[art]->model->visible)
   {
   cerr << "Not Visible\n";
   _query[q]->artifacts[art]->patmt->setNodeMask(0);
   _query[q]->artifacts[art]->model->so->attachToScene();
   _query[q]->artifacts[art]->model->visible = true;
_query[q]->artifacts[art]->model->visibleMap->setValue(true);
                 activateModelFromQuery(q, art);
   }
}
else
{
int inc = art;
//Load Model
cerr << "Loading New\n";

_query[q]->artifacts[art]->model = new SelectModel; 

//Set Variables
cerr << "Setting Variables\n";

        string basket = _query[q]->artifacts[art]->values[1];
        string modelPath = "";

        _query[q]->artifacts[art]->model->pos = _query[q]->artifacts[art]->modelPos;
        _query[q]->artifacts[art]->model->orig = _query[q]->artifacts[art]->modelPos;
        _query[q]->artifacts[art]->model->rot = Quat(0,0,0,1);

//Get Path for ScanModel
cerr << "Get Path for ScanModel\n";
        modelPath = ConfigManager::getEntry("Plugin.ArtifactVis2.3DModelFolder").append("data/scan_models/" + basket + "/" + basket + ".ply");

        if (modelExists(modelPath.c_str()))
        {
	   _query[q]->artifacts[art]->model->scanModel = modelPath;
	   _query[q]->artifacts[art]->model->scanScale = 0.005;       
           cerr << "Found Model\n";
	}
        else
        {
	   _query[q]->artifacts[art]->model->scanModel = "";
	   _query[q]->artifacts[art]->model->scanScale = 1;       
	}
           _query[q]->artifacts[art]->model->scanPos = _query[q]->artifacts[art]->model->pos;
           _query[q]->artifacts[art]->model->scanRot = _query[q]->artifacts[art]->model->rot;

//Get Path for DCModel

        string dc;
        dc = _query[q]->artifacts[art]->dc;
        dc.erase(2,dc.size());
	   _query[q]->artifacts[art]->model->dcModel = dc;
	   _query[q]->artifacts[art]->model->dcScale = 0.05;
           _query[q]->artifacts[art]->model->dcPos = _query[q]->artifacts[art]->model->pos;
           _query[q]->artifacts[art]->model->dcRot = _query[q]->artifacts[art]->model->rot;

//Get Path for cubeModel
            modelPath = ConfigManager::getEntry("Plugin.ArtifactVis2.3DModelFolder").append("photos/" + basket + "/test.obj");

        if (modelExists(modelPath.c_str()))
        {
	   _query[q]->artifacts[art]->model->cubeModel = modelPath;
	   _query[q]->artifacts[art]->model->cubeScale = 0.05;       
           cerr << "Found Photos\n";
	}
        else
        {
	   _query[q]->artifacts[art]->model->cubeModel = "";
	   _query[q]->artifacts[art]->model->cubeScale = 1;       
	}
           _query[q]->artifacts[art]->model->cubePos = _query[q]->artifacts[art]->model->pos;
           _query[q]->artifacts[art]->model->cubeRot = _query[q]->artifacts[art]->model->rot;

//Get Path for frameModel
            modelPath = ConfigManager::getEntry("Plugin.ArtifactVis2.3DModelFolder").append("photos/" + basket + "/frame.obj");

        if (modelExists(modelPath.c_str()))
        {
	   _query[q]->artifacts[art]->model->frameModel = modelPath;
	   _query[q]->artifacts[art]->model->frameScale = 0.005;       
           cerr << "Found Frame\n";
	}
        else
        {
	   _query[q]->artifacts[art]->model->frameModel = "";
	   _query[q]->artifacts[art]->model->frameScale = 1;       
	}
	
	if(_query[q]->artifacts[art]->model->frameModel != "")
        {
                string file = ConfigManager::getEntry("Plugin.ArtifactVis2.3DModelFolder").append("photos/" + basket + "/frame.xml");
                FILE* fp = fopen(file.c_str(), "r");
                if(fp != NULL)
                {
                mxml_node_t* tree;
                tree = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
                fclose(fp);
                mxml_node_t* child;
                float trans[3];
                float scale[3];
                float rotDegrees[4];
                child = mxmlFindElement(tree, tree, "easting", NULL, NULL, MXML_DESCEND);
                trans[0] = atof(child->child->value.text.string);
                child = mxmlFindElement(tree, tree, "northing", NULL, NULL, MXML_DESCEND);
                trans[1] = atof(child->child->value.text.string);
                child = mxmlFindElement(tree, tree, "elevation", NULL, NULL, MXML_DESCEND);
                trans[2] = atof(child->child->value.text.string);
                child = mxmlFindElement(tree, tree, "x", NULL, NULL, MXML_DESCEND);
                scale[0] = atof(child->child->value.text.string);
                child = mxmlFindElement(tree, tree, "y", NULL, NULL, MXML_DESCEND);
                scale[1] = atof(child->child->value.text.string);
                child = mxmlFindElement(tree, tree, "z", NULL, NULL, MXML_DESCEND);
                scale[2] = atof(child->child->value.text.string);
                child = mxmlFindElement(tree, tree, "heading", NULL, NULL, MXML_DESCEND);
                rotDegrees[0] = atof(child->child->value.text.string);
                child = mxmlFindElement(tree, tree, "tilt", NULL, NULL, MXML_DESCEND);
                rotDegrees[1] = atof(child->child->value.text.string);
                child = mxmlFindElement(tree, tree, "roll", NULL, NULL, MXML_DESCEND);
                rotDegrees[2] = atof(child->child->value.text.string);
		child = mxmlFindElement(tree, tree, "w", NULL, NULL, MXML_DESCEND);
		bool degrees = true;
		if(child != NULL)
		{
		rotDegrees[3] = atof(child->child->value.text.string);
		degrees = false;
		}
		Quat rot;
		if(degrees)
		{

		rotDegrees[0] = DegreesToRadians(rotDegrees[0]);
		rotDegrees[1] = DegreesToRadians(rotDegrees[1]);
		rotDegrees[2] = DegreesToRadians(rotDegrees[2]);
		rot = osg::Quat(rotDegrees[0], osg::Vec3d(1,0,0),rotDegrees[1], osg::Vec3d(0,1,0),rotDegrees[2], osg::Vec3d(0,0,1)); 
		}
		else
		{
		  cerr << "As Quats\n";
		  rot = Quat(rotDegrees[0],rotDegrees[1],rotDegrees[2],rotDegrees[3]);
		}
           if(trans[0] != 0)
           {                 
	   _query[q]->artifacts[art]->model->framePos = Vec3(trans[0], trans[1], trans[2]);       
	   }
           else
           {
           _query[q]->artifacts[art]->model->framePos = _query[q]->artifacts[art]->model->pos;
           }
           _query[q]->artifacts[art]->model->frameRot = rot;
	   _query[q]->artifacts[art]->model->frameScale = scale[0];       

         }
	}
        else
        {
           _query[q]->artifacts[art]->model->framePos = _query[q]->artifacts[art]->model->pos;
           _query[q]->artifacts[art]->model->frameRot = _query[q]->artifacts[art]->model->rot;

	}

//Find Preferred Model
//50674
string currentModelPath;
float currentScale;
osg::Vec3 currentPos;     
osg::Quat currentRot;     
string currentModelType;

if(_query[q]->artifacts[art]->model->scanModel != "")
{
currentModelPath = _query[q]->artifacts[art]->model->scanModel;
currentScale =  _query[q]->artifacts[art]->model->scanScale;
currentPos =   _query[q]->artifacts[art]->model->scanPos;     
currentRot =   _query[q]->artifacts[art]->model->scanRot;     
currentModelType = "scan";
_query[q]->artifacts[art]->model->currentModelType = currentModelType;
}
else if (_query[q]->artifacts[art]->model->cubeModel != "")
{
currentModelPath = _query[q]->artifacts[art]->model->cubeModel;
currentScale =  _query[q]->artifacts[art]->model->cubeScale;
currentPos =   _query[q]->artifacts[art]->model->cubePos;     
currentRot =   _query[q]->artifacts[art]->model->cubeRot;     
currentModelType = "cube";
_query[q]->artifacts[art]->model->currentModelType = currentModelType;
}
else if (_query[q]->artifacts[art]->model->frameModel != "")
{
currentModelPath = _query[q]->artifacts[art]->model->frameModel;
currentScale =  _query[q]->artifacts[art]->model->frameScale;
currentPos =   _query[q]->artifacts[art]->model->framePos;     
currentRot =   _query[q]->artifacts[art]->model->frameRot;     
currentModelType = "frame";
_query[q]->artifacts[art]->model->currentModelType = currentModelType;

}
else
{
currentModelPath = _query[q]->artifacts[art]->model->dcModel;
currentScale =  _query[q]->artifacts[art]->model->dcScale;
currentPos =   _query[q]->artifacts[art]->model->dcPos;     
currentRot =   _query[q]->artifacts[art]->model->dcRot;     
currentModelType = "dc";
_query[q]->artifacts[art]->model->currentModelType = currentModelType;
}


//Load Model into Node

Node* modelNode;

    int index = convertDCtoIndex(dc);
    _query[q]->artifacts[art]->model->dcIndex = index;

if(currentModelType == "dc")
{
    if(_modelLoaded[index])
    {
      modelNode = _models[index];

    }
    else
    {
	modelNode = defaultDcModel;
    }
}
else
{
            if (objectMap.count(currentModelPath) == 0)
	    {
		 objectMap[currentModelPath] = osgDB::readNodeFile(currentModelPath);
	    }
            modelNode = objectMap[currentModelPath];
}

if(modelNode != NULL)
{
  cerr << "Model Loaded\n";
}
else
{
  cerr << "Model Failed to Load\n";

}
//Add Lighting and Culling

		if(false)
		{
		    osg::StateSet* stateset = modelNode->getOrCreateStateSet();
		    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
		}
		if(false)
		{
		    osg::StateSet * stateset = modelNode->getOrCreateStateSet();
		    osg::CullFace * cf=new osg::CullFace();
		    cf->setMode(osg::CullFace::BACK);
		    stateset->setAttributeAndModes( cf, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
		}
                if(false)
		{
		TextureResizeNonPowerOfTwoHintVisitor tr2v(false);
		modelNode->accept(tr2v);
                }

                if(true)
                {
                    StateSet* ss = modelNode->getOrCreateStateSet();
                    ss->setMode(GL_LIGHTING, StateAttribute::ON | osg::StateAttribute::OVERRIDE);
                    Material* mat = new Material();
                    mat->setColorMode(Material::AMBIENT_AND_DIFFUSE);
                    Vec4 color_dif(1, 1, 1, 1);
                    mat->setDiffuse(Material::FRONT_AND_BACK, color_dif);
                    ss->setAttribute(mat);
                    ss->setAttributeAndModes(mat, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
                }

//Add to SceneObject
     _query[q]->artifacts[inc]->model->name = basket;
    string name = basket;

	    SceneObject * so;
	    so = new SceneObject(name, false, false, false, true, false);
	    osg::Switch* switchNode = new osg::Switch();
	    so->addChild(switchNode);
	    PluginHelper::registerSceneObject(so,"Test");
	    so->attachToScene();
//Add currentNode to switchNode
      _query[q]->artifacts[inc]->model->currentModelNode = modelNode;  
	switchNode->addChild(_query[q]->artifacts[inc]->model->currentModelNode);
      _query[q]->artifacts[inc]->model->switchNode = switchNode;
//Add menu system
	    so->setNavigationOn(true);
	    so->setMovable(true);
	    so->addMoveMenuItem();
	    so->addNavigationMenuItem();

	    SubMenu * sm = new SubMenu("Position");
	    so->addMenuItem(sm);

	    MenuButton * mb;
	    mb = new MenuButton("Load");
	    mb->setCallback(this);
	    sm->addItem(mb);

	    SubMenu * savemenu = new SubMenu("Save");
	    sm->addItem(savemenu);

	    mb = new MenuButton("Save");
	    mb->setCallback(this);
	    savemenu->addItem(mb);
            _query[q]->artifacts[inc]->model->saveMap = mb;

	    mb = new MenuButton("Reset to Origin");
	    mb->setCallback(this);
	    so->addMenuItem(mb);
            _query[q]->artifacts[inc]->model->resetMap = mb;

            MenuCheckbox * mc;
	    mc = new MenuCheckbox("Active",true);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            _query[q]->artifacts[inc]->model->activeMap = mc;

            
	    mc = new MenuCheckbox("Visible",true);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            _query[q]->artifacts[inc]->model->visibleMap = mc;
            _query[q]->artifacts[inc]->model->visible = true;

	    mc = new MenuCheckbox("Panel Visible",true);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            _query[q]->artifacts[inc]->model->pVisibleMap = mc;
           // _query[q]->artifacts[inc]->model->pVisible = true;

         if(_query[q]->artifacts[art]->model->dcModel != "")
         {
            bool checked = false;
	    if(currentModelType == "dc") checked = true;
	    mc = new MenuCheckbox("3D Symbol",checked);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            _query[q]->artifacts[inc]->model->dcMap = mc;
	 }

         if(_query[q]->artifacts[art]->model->scanModel != "")
         {
            bool checked = false;
	    if(currentModelType == "scan") checked = true;
	    mc = new MenuCheckbox("3D Scan",checked);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            _query[q]->artifacts[inc]->model->scanMap = mc;
	 }

         if(_query[q]->artifacts[art]->model->cubeModel != "")
         {
            bool checked = false;
	    if(currentModelType == "cube") checked = true;
	    mc = new MenuCheckbox("Photograph Cube",checked);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            _query[q]->artifacts[inc]->model->cubeMap = mc;
         }

         if(_query[q]->artifacts[art]->model->frameModel != "")
         {
            bool checked = false;
	    if(currentModelType == "frame") checked = true;

	    SubMenu * photosmenu = new SubMenu("Photo Frames");
	    so->addMenuItem(photosmenu);

            //TODO:Turn into Array Loop
            string photoName0 = "Testing";
	    mc = new MenuCheckbox(photoName0,checked);
	    mc->setCallback(this);
	    photosmenu->addItem(mc);
            _query[q]->artifacts[inc]->model->photoMap.push_back(mc);
	 }

Vec3 orig = currentPos; 
cerr << "Pos: " << orig.x() << " " << orig.y() << " " << orig.z() << "\n";

//Mask artifact loaded by query 
    _query[q]->artifacts[inc]->patmt->setNodeMask(0);
 so->setPosition(currentPos);     
 so->setScale(currentScale);
 so->setRotation(currentRot);     



    _query[q]->artifacts[inc]->model->so = so;
    _query[q]->artifacts[inc]->model->pos = so->getPosition();
    _query[q]->artifacts[inc]->model->rot = so->getRotation();

cerr << "Pass\n";

    ArtifactAnnoTrack* artifactModelTrack = new ArtifactAnnoTrack;
    artifactModelTrack->active = true;
     artifactModelTrack->q = q; 
     artifactModelTrack->art = inc; 
    _artifactModelTrack.push_back(artifactModelTrack);
}
}




int ArtifactVis2::convertDCtoIndex(string dc)
{
int index = -1;

string c1 = dc;
c1.erase(1,1);
cerr << c1 << "\n";
string c2 = dc;
c2.erase(0,1);
cerr << c2 << "\n";



std::string ascii[] = {"A", "B", "C", "D", "E", "F", "G" , "H" , "I" , "J" , "K" , "L" , "M" , "N" , "O" , "P" , "Q" , "R" , "S" , "T" , "U" , "V" , "W" , "X" , "Y" , "Z" };

 int a = -1;
 int b = -1;

    for (int i = 0; i < 26; i++)
    {
   	if(c1 == ascii[i])
   	{
          a = i;	
      	
   	}
   	if(c2 == ascii[i])
   	{
          b = i;	
      	
   	}

    }

    if(a != -1 && b != -1)
    {
       index = a * 26 + b;
    }

return index;
}
void ArtifactVis2::switchModelType(string type, int q, int art)
{
if(type != _query[q]->artifacts[art]->model->currentModelType)
{
    deactivateModelSwitches(q, art);
   string oldType = _query[q]->artifacts[art]->model->currentModelType;

   float currentScale = 0.1;
   Quat currentRot;

Node* modelNode;
 if(type == "dc")
 {

    int index = _query[q]->artifacts[art]->model->dcIndex;
    if(_modelLoaded[index])
    {
      modelNode = _models[index];

    }
    else
    {
	modelNode = defaultDcModel;
    }
   currentScale =  _query[q]->artifacts[art]->model->dcScale;
   currentRot =  _query[q]->artifacts[art]->model->dcRot;
   _query[q]->artifacts[art]->model->dcMap->setValue(true);
 }
 else
 {
     string currentModelPath; 
     if(type == "scan")
     {
        currentScale =  _query[q]->artifacts[art]->model->scanScale;
        _query[q]->artifacts[art]->model->scanMap->setValue(true);
        currentModelPath = _query[q]->artifacts[art]->model->scanModel; 
        currentRot =  _query[q]->artifacts[art]->model->scanRot;
     }
     if(type == "cube")
     {
	 currentScale =  _query[q]->artifacts[art]->model->cubeScale;
        _query[q]->artifacts[art]->model->cubeMap->setValue(true);
        currentModelPath = _query[q]->artifacts[art]->model->cubeModel; 
        currentRot =  _query[q]->artifacts[art]->model->cubeRot;
     }
     if(type == "frame")
     {
	 currentScale =  _query[q]->artifacts[art]->model->frameScale;
	//TODO: Handle for Array
        _query[q]->artifacts[art]->model->photoMap[0]->setValue(true);
        currentModelPath = _query[q]->artifacts[art]->model->frameModel; 
        currentRot =  _query[q]->artifacts[art]->model->frameRot;
     }

            if (objectMap.count(currentModelPath) == 0)
	    {
		 objectMap[currentModelPath] = osgDB::readNodeFile(currentModelPath);
	    }
            modelNode = objectMap[currentModelPath];
 }
 if(modelNode != NULL)
 {
//Add Lighting and Culling

		if(true)
		{
		    osg::StateSet* stateset = modelNode->getOrCreateStateSet();
		    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
		}
		if(true)
		{
		    osg::StateSet * stateset = modelNode->getOrCreateStateSet();
		    osg::CullFace * cf=new osg::CullFace();
		    cf->setMode(osg::CullFace::BACK);
		    stateset->setAttributeAndModes( cf, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
		}
                if(true)
		{
		TextureResizeNonPowerOfTwoHintVisitor tr2v(false);
		modelNode->accept(tr2v);
                }

    _query[q]->artifacts[art]->model->switchNode->removeChild(_query[q]->artifacts[art]->model->currentModelNode);
    _query[q]->artifacts[art]->model->currentModelNode = modelNode;  
    _query[q]->artifacts[art]->model->switchNode->addChild(_query[q]->artifacts[art]->model->currentModelNode);
    _query[q]->artifacts[art]->model->so->setScale(currentScale);
    _query[q]->artifacts[art]->model->so->setRotation(currentRot);

    _query[q]->artifacts[art]->model->currentModelType = type;
 }

}  
}
void ArtifactVis2::deactivateModelSwitches(int q, int art)
{

        if(_query[q]->artifacts[art]->model->dcMap != NULL)
        {   
          _query[q]->artifacts[art]->model->dcMap->setValue(false);
        }
        if(_query[q]->artifacts[art]->model->scanMap != NULL)
        {   
          _query[q]->artifacts[art]->model->scanMap->setValue(false);
        }
        if(_query[q]->artifacts[art]->model->cubeMap != NULL)
        {   
          _query[q]->artifacts[art]->model->cubeMap->setValue(false);
        }
        if(_query[q]->artifacts[art]->model->photoMap.size() > 0)
        {   
          _query[q]->artifacts[art]->model->photoMap[0]->setValue(false);
        }
}
void ArtifactVis2::menuSetup()
{
    //Menu Setup:

    _infoPanel = new TabbedDialogPanel(300, 30, 4, "ArtifactVis2", "Plugin.ArtifactVis2.InfoPanel");
    _infoPanel->setVisible(true);

    _infoPanel->addTextTab("Default", "");
    _infoPanel->addTextTab("Artifacts", "");
    _infoPanel->addTextTab("Loci", "");
    _infoPanel->addTextTab("BookMarks", "");
    _infoPanel->setActiveTab("Default");


    _displayMenu = new SubMenu("Display");


    _utilsMenu = new MenuCheckbox("Utils", false);
    _utilsMenu->setCallback(this);
   _infoPanel->addMenuItem(_utilsMenu);
    _qsMenu = new MenuCheckbox("Query System", false);
    _qsMenu->setCallback(this);
   _infoPanel->addMenuItem(_qsMenu);
    _bookmarksMenu = new MenuCheckbox("Bookmarks", false);
    _bookmarksMenu->setCallback(this);
   _infoPanel->addMenuItem(_bookmarksMenu);
    _fileMenu = new MenuCheckbox("FileManager", false);
    _fileMenu->setCallback(this);
   _infoPanel->addMenuItem(_fileMenu);
    _artifactsDropDown = new MenuButton("Artifacts");
    _artifactsDropDown->setCallback(this);
    artifactsDropped = false; 
   _infoPanel->addMenuItem(_artifactsDropDown);
    _lociDropDown = new MenuButton("Loci");
    _lociDropDown->setCallback(this);
    lociDropped = false; 
   _infoPanel->addMenuItem(_lociDropDown);
   // setupHudMenu();
    //Generates the menus to toggle each query on/off.
    setupQuerySelectMenu();
    //Generates the menu for selecting models to load
    findAllModels();
    setupSiteMenu();
    //Generates the menus to query each table.
    setupTablesMenu();
    for (int i = 0; i < _tables.size(); i++)
    {
        setupQueryMenu(_tables[i]);
    }
    //Generates the menus to fly to coordinates.
   setupFlyToMenu();
   setupUtilsMenu();
   setupFileMenu();
    _picFolder = ConfigManager::getEntry("value", "Plugin.ArtifactVis2.PicFolder", "");
    //Tabbed dialog for selecting artifacts
    /*
    _artifactPanel = new TabbedDialogPanel(400, 30, 4, "Selected Artifact", "Plugin.ArtifactVis2.ArtifactPanel");
    _artifactPanel->addTextTab("Info", "");
    _artifactPanel->addTextureTab("Side", "");
    _artifactPanel->addTextureTab("Top", "");
    _artifactPanel->addTextureTab("Bottom", "");
    _artifactPanel->setVisible(false);
    _artifactPanel->setActiveTab("Info");
    _selectionStatsPanel = new DialogPanel(450, "Selection Stats", "Plugin.ArtifactVis2.SelectionStatsPanel");
    _selectionStatsPanel->setVisible(false);
    */

}
void ArtifactVis2::initSelectBox()
{
    //create wireframe selection box
    osg::Box* sbox = new osg::Box(osg::Vec3(0, 0, 0), 1.0, 1.0, 1.0);
    osg::ShapeDrawable* sd = new osg::ShapeDrawable(sbox);
    osg::StateSet* stateset = sd->getOrCreateStateSet();
    osg::PolygonMode* polymode = new osg::PolygonMode;
    polymode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
    stateset->setAttributeAndModes(polymode, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
    osg::Geode* geo = new osg::Geode();
    geo->addDrawable(sd);
    _selectBox = new osg::MatrixTransform();
    _selectBox->addChild(geo);

    // create select mark for wand
    osg::Sphere* ssph = new osg::Sphere(osg::Vec3(0, 0, 0), 10);
    sd = new osg::ShapeDrawable(ssph);
    sd->setColor(osg::Vec4(1.0, 0, 0, 1.0));
    stateset = sd->getOrCreateStateSet();
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    stateset->setAttributeAndModes(_defaultMaterial, osg::StateAttribute::ON);
    geo = new osg::Geode();
    geo->addDrawable(sd);
    _selectMark = new osg::MatrixTransform();
    _selectMark->addChild(geo);


}
void ArtifactVis2::secondInit()
{

    loadModels();

    //Algorithm for generating colors based on DC.
    for (int i = 0; i < 729; i++)
    {
        _colors[i] = Vec4(1 - float((i % 9) * 0.125), 1 - float(((i / 9) % 9) * 0.125), 1 - float(((i / 81) % 9) * 0.125), 1);
    }

    menuSetup();


    
    if(ConfigManager::getBool("Plugin.ArtifactVis2.MoveCamera"))
    {

    int flyIndex = ConfigManager::getInt("Plugin.ArtifactVis2.FlyToDefault");
    flyTo(flyIndex);
    }

    if(ConfigManager::getBool("Plugin.ArtifactVis2.MoveCamera") && true)
    {
    readAnnotationFile();
    }

    secondInitComplete = true;
    ArtifactVis2On = true;
}
void ArtifactVis2::addCheckBoxMenuItems(std::vector<cvr::MenuCheckbox*> checkBox)
{
    for (int i = 0; i < checkBox.size(); i++)
    {
        _infoPanel->addMenuItem(checkBox[i]);
       // _infoPanel->addMenuItem(showInfo);
    }

}
void ArtifactVis2::removeCheckBoxMenuItems(std::vector<cvr::MenuCheckbox*> checkBox)
{
    for (int i = 0; i < checkBox.size(); i++)
    {
        _infoPanel->removeMenuItem(checkBox[i]);
       // _infoPanel->addMenuItem(showInfo);
    }

}
void ArtifactVis2::addSubMenuItems(std::vector<cvr::SubMenu*> menu)
{
    for (int i = 0; i < menu.size(); i++)
    {
        _infoPanel->addMenuItem(menu[i]);
       // _infoPanel->addMenuItem(showInfo);
    }

}
void ArtifactVis2::removeSubMenuItems(std::vector<cvr::SubMenu*> menu)
{
    for (int i = 0; i < menu.size(); i++)
    {
        _infoPanel->removeMenuItem(menu[i]);
       // _infoPanel->addMenuItem(showInfo);
    }

}
void ArtifactVis2::updateDropDowns()
{
   
  _infoPanel->removeMenuItem(_lociDropDown);
  if(lociDropped)
  {
   removeCheckBoxMenuItems(_queryOptionLoci);
  }
  _infoPanel->removeMenuItem(_modelDropDown);
  if(modelDropped)
  {
    removeSubMenuItems(_modelMenus);
  }
  _infoPanel->removeMenuItem(_pcDropDown);
  if(pcDropped)
  {
    removeSubMenuItems(_pcMenus);
  }


  if(artifactsDropped)
  {
    addCheckBoxMenuItems(_queryOption);
  }
  else
  {
    removeCheckBoxMenuItems(_queryOption);
  }
  _infoPanel->addMenuItem(_lociDropDown);

  if(lociDropped)
  {
    addCheckBoxMenuItems(_queryOptionLoci);
  }
  else
  {
    removeCheckBoxMenuItems(_queryOptionLoci);
  }

  _infoPanel->addMenuItem(_modelDropDown);
  if(modelDropped)
  {
    addSubMenuItems(_modelMenus);
  }
  else
  {
    removeSubMenuItems(_modelMenus);
  }
  _infoPanel->addMenuItem(_pcDropDown);

  if(pcDropped)
  {
    addSubMenuItems(_pcMenus);
  }
  else
  {
    removeSubMenuItems(_pcMenus);
  }
}
void ArtifactVis2::setupUtilsMenu()
{
    _utilsPanel = new TabbedDialogPanel(100, 30, 4, "Utils", "Plugin.ArtifactVis2.UtilsPanel");
    _utilsPanel->setVisible(false);

    _createAnnotations = new MenuCheckbox("Create Annotations", false);
    _createAnnotations->setCallback(this);
    _utilsPanel->addMenuItem(_createAnnotations);

    _createMarkup = new MenuCheckbox("Create Markup", false);
    _createMarkup->setCallback(this);
    _utilsPanel->addMenuItem(_createMarkup);

    _bookmarkLoc = new MenuButton("Save Location");
    _bookmarkLoc->setCallback(this);
    _utilsPanel->addMenuItem(_bookmarkLoc);
    _selectArtifactCB = new MenuCheckbox("Select Artifact", true);
    _selectArtifactCB->setCallback(this);
    _utilsPanel->addMenuItem(_selectArtifactCB);
    _manipArtifactCB = new MenuCheckbox("Manipulate Artifact", false);
    _manipArtifactCB->setCallback(this);
    _utilsPanel->addMenuItem(_manipArtifactCB);
    _scaleBar = new MenuCheckbox("Scale Bar", false);  //new
    _scaleBar->setCallback(this);
    _utilsPanel->addMenuItem(_scaleBar);
    _selectCB = new MenuCheckbox("Select box", false);
    _selectCB->setCallback(this);
    _utilsPanel->addMenuItem(_selectCB);
}
void ArtifactVis2::getDirFiles(const string& dirname, std::vector<DirFile*> & entries, string types)
{

    if(types == "")
    {
     types = ConfigManager::getEntry("Plugin.ArtifactVis2.FileManagerTypes"); 
    }

direct ** darray;
    int entryCount = scandir(const_cast<char*>(dirname.c_str()),
			     &darray, 0, alphasort);

    for (int k = 0; k < entryCount; k++)
    {
        DirFile* entry = new DirFile();
        string filename = darray[k]->d_name;
        if(filename != ".")
        {
        //cout << "Filename: " << filename << endl;
        entry->filename = filename; 
        entry->path = dirname;
        string origDir = dirname;
        origDir.append(filename);
        struct stat info;
        lstat(origDir.c_str(), &info);
        if(S_ISDIR(info.st_mode))
        {
         //cout << filename << " is a directory\n";
         entry->filetype = "folder"; 
        }
        else
        {
            size_t found=filename.find(".");
            if (found!=string::npos)
	    {
                 int start = int(found);
                 string tempFile = filename;
                 tempFile.erase(0,(start+1));                 
                 entry->filetype = tempFile; 
                 //cout <<" type: " << tempFile << endl;
            }
            else
            {
              entry->filetype = ""; 
            }
        }
        if(types != "")
        { 
        	size_t found=types.find(entry->filetype);
        	if(entry->filetype == "folder")
        	{
        	entries.push_back(entry);
        	}
        	else if (found!=string::npos)
		{
                 if(entry->filetype != "")
                 { 
        	   entries.push_back(entry);
                 }
        	}
        }
        else
        {
        entries.push_back(entry); 
        }

        }
    }

}
void ArtifactVis2::setupFileMenu()
{

    string dir = ConfigManager::getEntry("Plugin.ArtifactVis2.3DModelFolder");

    _currentDir = dir;
    entries.clear();
    string types = "";
    getDirFiles(dir, entries,types);

    _filePanel = new PopupMenu("FileManager", "Plugin.ArtifactVis2.FileManagerPanel");
    _filePanel->setVisible(false);
   
    _resetFileManager = new MenuButton("Reset to Home Directory");
    _resetFileManager->setCallback(this);
    _filePanel->addMenuItem(_resetFileManager);

    _upFileManager = new MenuButton("--Scroll Up--");
    _upFileManager->setCallback(this);
    _filePanel->addMenuItem(_upFileManager);

     int i = 0;
     int count = entries.size();
     if(count > 10)
     {
       count = 10 + i;
     }
     else
     {
       i = 0;

     }
     if(count > entries.size())
     {
      count = entries.size();
      i = count - 10;
     }

    for (i = 0; i < count; i++)
    {
    string filename = entries[i]->filename;
    if(entries[i]->filetype == "folder")
    {
       filename.append("/");
    }
    cvr::MenuButton* entry = new MenuButton(filename);
    entry->setCallback(this);
    fileButton.push_back(entry);
    _filePanel->addMenuItem(entry);

    }
    _downFileManager = new MenuButton("--Scroll Down--");
    _downFileManager->setCallback(this);
    _filePanel->addMenuItem(_downFileManager);

}
void ArtifactVis2::updateFileMenu(std::string dir, int scroll)
{

    
      _filePanel->removeMenuItem(_downFileManager);
    for (int i = 0; i < fileButton.size(); i++)
    {
    _filePanel->removeMenuItem(fileButton[i]);
    }
    fileButton.clear();

    entries.clear();
    
    string types = "";
    getDirFiles(dir, entries, types);
     
     int i = scroll * 10;
     int count = entries.size();
     
     if(count > 10)
     {
       count = 10 + i;
     }
     else
     {
       i = 0;

     }
     if(count > entries.size())
     {
      count = entries.size();
      i = count - 10;
     }
    for (i; i < count; i++)
    {
    string filename = entries[i]->filename;
    if(entries[i]->filetype == "folder")
    {
       filename.append("/");
    }
    cvr::MenuButton* entry = new MenuButton(filename);
    entry->setCallback(this);
    fileButton.push_back(entry);
    _filePanel->addMenuItem(entry);

    }

    _downFileManager = new MenuButton("--Scroll Down--");
    _downFileManager->setCallback(this);
    _filePanel->addMenuItem(_downFileManager);
}
void ArtifactVis2::addNewPC(int index)
{
    Vec3Array* coords;
    Vec4Array* colors;
    std::vector<Vec3> coords2;
    std::vector<Vec4> colors2;

 int i = index;
 newFileAvailable = false;
 string currentModelPath = _pointClouds[i]->fullpath;
 string name = _pointClouds[i]->name;
 newSelectedFile = "";
 newSelectedName = "";

    string filename = currentModelPath;
   string type = filename;
cerr << "Mark1\n";
    bool nvidia = ConfigManager::getBool("Plugin.ArtifactVis2.Nvidia");
            cout << "File: " << filename << endl;
            PointsLoadInfo pli;
            pli.file = filename;
            pli.group = new osg::Group();
    if (true)
    {
        coords = new Vec3Array();
        colors = new Vec4Array();
        cout << "Reading point cloud data from : " << filename <<  "..." << endl;
        ifstream file(filename.c_str());
        type.erase(0, (type.length() - 3));
        cout << type << "\n";
        string line;
        bool read = false;
       // cout << _pcFactor[index] << "\n";
       // int factor = _pcFactor[index];
       int factor = 1;
        if((type == "ply" || type == "xyb") && nvidia)
        {
            cout << "Reading XYB" << endl;

            //float x,y,z;
            //bamop
            float scale = 9;
            //	osg::Vec3 offset(transx, transy, transz);
            //_activeObject = new PointsObject("Points",true,false,false,true,true);



            PluginHelper::sendMessageByName("Points",POINTS_LOAD_REQUEST,(char*)&pli);

            if(!pli.group->getNumChildren())
            {
                std::cerr << "PointsWithPans: Error, no points loaded for file: " << pli.file << std::endl;
                return;
            }

            float pscale;
            if(type == "xyb")
            {
                pscale = 100;
            }
            else
            {
                pscale = 0.3;
            }
            pscale = ConfigManager::getFloat("Plugin.ArtifactVis2.scaleP", 0);
            cerr << "Scale of P is " << pscale << "\n";
//StartHereToday
            osg::Uniform*  _scaleUni = new osg::Uniform("pointScale",1.0f * pscale);
            pli.group->getOrCreateStateSet()->addUniform(_scaleUni);
            //_activeObject->attachToScene();
            //_activeObject->setNavigationOn(false);
            //_activeObject->setScale(scale);
            //_activeObject->setPosition(_pcPos[index]);
            //_activeObject->setNavigationOn(true);
            //_activeObject->addChild(pli.group.get());
            /*
            MatrixTransform* rotTransform = new MatrixTransform();
            Matrix pitchMat;
            Matrix yawMat;
            Matrix rollMat;
            pitchMat.makeRotate(DegreesToRadians(_pcRot[index].x()), 1, 0, 0);
            yawMat.makeRotate(DegreesToRadians(_pcRot[index].y()), 0, 1, 0);
            rollMat.makeRotate(DegreesToRadians(_pcRot[index].z()), 0, 0, 1);
            rotTransform->setMatrix(pitchMat * yawMat * rollMat);
            MatrixTransform* scaleTransform = new MatrixTransform();
            Matrix scaleMat;
            scaleMat.makeScale(_pcScale[index]);
            scaleTransform->setMatrix(scaleMat);
            MatrixTransform* posTransform = new MatrixTransform();
            Matrix posMat;
            Matrix invertMat;
            posMat.makeTranslate(_pcPos[index]);
            posTransform->setMatrix(posMat);
            MatrixTransform* invTransform = new MatrixTransform();
            rotTransform->addChild(pli.group.get());
            cerr << "Num childs " << pli.group.get()->getNumChildren() << "\n";
            scaleTransform->addChild(rotTransform);
            posTransform->addChild(scaleTransform);

            _pcRoot[index] = new MatrixTransform();
            _pcRoot[index]->addChild(posTransform);
            cout << "Loaded" << endl;
            cerr << "Num childs2 " << _pcRoot[index]->getNumChildren() << "\n";
            */
        }
        else
        {
            if (file.is_open())
            {
                int pcount = 0;
                int bten = factor - 1;
                int cptx = 0;
                double transx = 0;
                double transy = 0;
                double transz = 0;

                while (file.good())
                {
                    getline(file, line);
                    string lineout = line;

                    if (type == "ptx")
                    {
                        vector<string> entries;
                        //vector<string>array;
                        string token;
                        //string tmp = "this@is@a@line";
                        istringstream iss(line);
                        char lim = ' ';

                        while ( getline(iss, token, lim) )
                        {
                            entries.push_back(token);
                        }

                        if (entries.size() < 7)
                        {
                            cout << lineout << "\n";

                            if (cptx == 9)
                            {
                                cptx = 0;
                                //cout << lineout << "\n";
                                std::stringstream ss;
                                ss.precision(19);
                                transx = atof(entries[0].c_str());
                                transy = atof(entries[0].c_str());
                                transz = atof(entries[0].c_str());
                            }
                            else
                            {
                                cptx++;
                            }
                        }
                        else
                        {
                            std::stringstream ss;
                            ss.precision(19);
                            double x = atof(entries[0].c_str()) + transx;
                            double y = atof(entries[1].c_str()) + transy;
                            double z = atof(entries[2].c_str()) + transz;
                            double r = atof(entries[4].c_str()) / 255;
                            double g = atof(entries[5].c_str()) / 255;
                            double b = atof(entries[6].c_str()) / 255;
                            coords->push_back(Vec3d(x, y, z));
                           // _avgOffset[index] += Vec3d(x, y, z);
                            colors->push_back(Vec4d(r, g, b, 1.0));
                            bten = 0;
                            pcount++;
                        }
                    }

                    if (line.empty()) break;

                    if (read)
                    {
                        bten++;
                        vector<string> entries;
                        int ck = 0;

                        /*
                        while(true)
                        {
                            if(line.find(" ")==line.rfind(" "))
                                break;
                            else
                            {
                                entries.push_back(line.substr(0,line.find(" ")));
                                line = line.substr(line.find(" ")+1);
                                cout << line << "\n";
                            }
                        }
                        */
                        if (bten == factor)
                        {
                            while (ck < 7)
                            {
                                entries.push_back(line.substr(0, line.find(" ")));
                                line = line.substr(line.find(" ") + 1);
                                //cout << line << "\n";
                                ck++;
                            }
                        }

                        //cout << entries.size();
                        if ((type == "ply") && (bten == factor))
                        {
                            float x = atof(entries[0].c_str());
                            float y = atof(entries[1].c_str());
                            float z = atof(entries[2].c_str());
                            float r = atof(entries[3].c_str()) / 255;
                            float g = atof(entries[4].c_str()) / 255;
                            float b = atof(entries[5].c_str()) / 255;
                            coords->push_back(Vec3d(x, y, z));
                           // coords2.push_back(Vec3(x,y,z));
                           // _avgOffset[index] += Vec3d(x, y, z);
                            colors->push_back(Vec4f(r, g, b, 1.0));
                           // colors2.push_back(Vec4(r,g,b,1.0));
                            pcount++;
                            bten = 0;
                        }

                        if ((type == "txt") && (bten == factor))
                        {
                            float x = atof(entries[0].c_str());
                            float y = atof(entries[1].c_str());
                            float z = atof(entries[2].c_str());
                            float r = atof(entries[3].c_str()) / 255;
                            float g = atof(entries[4].c_str()) / 255;
                            float b = atof(entries[5].c_str()) / 255;
                            coords->push_back(Vec3d(x, y, z));
                           // _avgOffset[index] += Vec3d(x, y, z);
                            colors->push_back(Vec4f(r, g, b, 1.0));
                            pcount++;
                            bten = 0;
                        }
                        else if ((type == "pts") && (bten == factor))
                        {
                            std::stringstream ss;
                            ss.precision(19);
                            double x = atof(entries[0].c_str());
                            double y = atof(entries[1].c_str());
                            double z = atof(entries[2].c_str());
                            double r = atof(entries[4].c_str()) / 255;
                            double g = atof(entries[5].c_str()) / 255;
                            double b = atof(entries[6].c_str()) / 255;
                            coords->push_back(Vec3d(x, y, z));
                           // _avgOffset[index] += Vec3d(x, y, z);
                            colors->push_back(Vec4d(r, g, b, 1.0));
                            bten = 0;
                            pcount++;

                            if (pcount == 7216)
                            {
                                //std::stringstream ss;
                                //ss.precision(19);
                                printf("TOP %f %f %f\n", x, y, z);
                                cout << "Coords: " << entries[0].c_str() << " " << entries[1].c_str() << " " << entries[2].c_str() << "\n";
                                //cout << "Coords: " << coords[7216][0] << " " << coords[7216][1] << " " << coords[7216][2] << "\n";
                            }
                        }
                    }

                    if (type == "pts")
                    {
                        read = true;
                    }

                    if (line == "end_header")
                        read = true;
                }

                cout << "Finished Loading, Total Vertices: " << pcount << "\n";
            }
            else
            {
                cout << "Unable to open file: " << filename << endl;
                return;
            }
        }
    }
    Geode* pointGeode = new Geode();
    osg::Geode* sphereGeode = new Geode();  
    if(!nvidia)
    {


//        Geometry* pointCloud = new Geometry();
  //      pointCloud->setVertexArray(coords);
    //    pointCloud->setColorArray(colors);
      //  pointCloud->setColorBinding(Geometry::BIND_PER_VERTEX);
      //  DrawElementsUInt* points = new DrawElementsUInt(PrimitiveSet::POINTS, 0);

//bangSph 

   // Sphere* sphereShape;
  //  ShapeDrawable* shapeDrawable;

//        for (int n = 0; n < coords->size(); n++)
  //      {
            /*
            Sphere* sphereShape =  new Sphere(coords2[n], _vertexRadius);
            ShapeDrawable* shapeDrawable = new ShapeDrawable(sphereShape);
            // shapeDrawable->setTessellationHints(hints);
            shapeDrawable->setColor(colors2[n]);
            sphereGeode->addDrawable(shapeDrawable);
            */

             
       // pointCloud->addPrimitiveSet(points);
         //   points->push_back(n);
    //    }

 //   pointCloud->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0,coords->size()));
     //   pointCloud->addPrimitiveSet(points);
   //     pointGeode->addDrawable(pointCloud);
     if(true)
     {
        StateSet* ss = pli.group->getOrCreateStateSet();
        ss->setMode(GL_LIGHTING, StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
    }
    else
    {
    float size = 10;
    osg::StateSet* set = pli.group->getOrCreateStateSet();

    /// Setup cool blending
   // set->setMode(GL_LIGHTING,  osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    set->setMode(GL_BLEND, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    osg::BlendFunc *fn = new osg::BlendFunc();
   fn->setFunction(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::DST_ALPHA);
  // fn->setFunction(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE);
    //fn->setFunction(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    set->setAttributeAndModes(fn, osg::StateAttribute::ON);

    /// Setup the point sprites
    osg::PointSprite *sprite = new osg::PointSprite();
    set->setTextureAttributeAndModes(0, sprite, osg::StateAttribute::ON);

    /// Give some size to the points to be able to see the sprite
    osg::Point *pointz = new osg::Point();
    pointz->setSize(size);
    set->setAttribute(pointz);

    /// Disable depth test to avoid sort problems and Lighting
    //set->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    set->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    /// The texture for the sprites
    osg::Texture2D *tex = new osg::Texture2D();
    tex->setImage(osgDB::readImageFile("/home/calvr/osgdata/Images/particle.rgb"));
    set->setTextureAttributeAndModes(0, tex, osg::StateAttribute::ON);
}

     }
        if(true)
        {
            i = index;
float currentScale = _pointClouds[i]->scale;
      
	    SceneObject * so;
	    so = new SceneObject(name, false, false, false, true, false);
	    osg::Switch* switchNode = new osg::Switch();
	    so->addChild(switchNode);
	    PluginHelper::registerSceneObject(so,"Test");
	    so->attachToScene();
//Add currentNode to switchNode
     // _models3d[i]->currentModelNode = modelNode; 
     cerr << "here\n"; 
        if(!nvidia)
        {
	//switchNode->addChild(pointGeode);
	switchNode->addChild(pointGeode);
        }
        else
        {
        StateSet* ss = pli.group->getOrCreateStateSet();
        ss->setMode(GL_LIGHTING, StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
          
	switchNode->addChild(pli.group.get());

        }
     // _pointClouds[i]->switchNode = switchNode;

//Add menu system
	    so->setNavigationOn(true);
	    so->setMovable(false);
	    so->addMoveMenuItem();
	    so->addNavigationMenuItem();
            float min = 0.0001;
            float max = 1;
            so->addScaleMenuItem("Scale",min,max,currentScale);
	    SubMenu * sm = new SubMenu("Position");
	    so->addMenuItem(sm);

	    MenuButton * mb;
	    mb = new MenuButton("Load");
	    mb->setCallback(this);
	    sm->addItem(mb);

	    SubMenu * savemenu = new SubMenu("Save");
	    sm->addItem(savemenu);

	    mb = new MenuButton("Save");
	    mb->setCallback(this);
	    savemenu->addItem(mb);
            _pointClouds[i]->saveMap = mb;

	    mb = new MenuButton("Save New Kml");
	    mb->setCallback(this);
	    savemenu->addItem(mb);
            _pointClouds[i]->saveNewMap = mb;

	    mb = new MenuButton("Reset to Origin");
	    mb->setCallback(this);
	    so->addMenuItem(mb);
            _pointClouds[i]->resetMap = mb;

            MenuCheckbox * mc;
	    mc = new MenuCheckbox("Active",false);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            _pointClouds[i]->activeMap = mc;

            
	    mc = new MenuCheckbox("Visible",true);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            _pointClouds[i]->visibleMap = mc;
            _pointClouds[i]->visible = true;

            float rValue = 0;
            min = -1;
            max = 1;
            MenuRangeValue* rt = new MenuRangeValue("rx",min,max,rValue);
            rt->setCallback(this);
	    so->addMenuItem(rt);
            _pointClouds[i]->rxMap = rt;

            rt = new MenuRangeValue("ry",min,max,rValue);
            rt->setCallback(this);
	    so->addMenuItem(rt);
            _pointClouds[i]->ryMap = rt;

            rt = new MenuRangeValue("rz",min,max,rValue);
            rt->setCallback(this);
	    so->addMenuItem(rt);
            _pointClouds[i]->rzMap = rt;
/*
	    mc = new MenuCheckbox("Panel Visible",true);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
 //           _query[q]->artifacts[inc]->model->pVisibleMap = mc;
           // _query[q]->artifacts[inc]->model->pVisible = true;
*/
Quat currentRot = _pointClouds[i]->rot;
Vec3 currentPos = _pointClouds[i]->pos;
Vec3 orig = currentPos; 
cerr << "Pos: " << orig.x() << " " << orig.y() << " " << orig.z() << "\n";

 so->setPosition(currentPos);     
 so->setScale(currentScale);
 so->setRotation(currentRot);     

    _pointClouds[i]->so = so;
    _pointClouds[i]->pos = so->getPosition();
    _pointClouds[i]->rot = so->getRotation();
    _pointClouds[i]->active = false;
    _pointClouds[i]->loaded = true;
        }
    
}
void ArtifactVis2::addNewModel(int i)
{
 newFileAvailable = false;
 string currentModelPath = _models3d[i]->fullpath;
 string name = _models3d[i]->name;
 newSelectedFile = "";
 newSelectedName = "";

// Matrix handMat = getHandToObjectMatrix();
         Vec3 currentPos = _models3d[i]->pos;
        Quat  currentRot = _models3d[i]->rot;
  //Check if ModelPath has been loaded
  Node* modelNode;
  
            if (objectMap.count(currentModelPath) == 0)
	    {
		 objectMap[currentModelPath] = osgDB::readNodeFile(currentModelPath);
	    }
            modelNode = objectMap[currentModelPath];
  
//Add Lighting and Culling

		if(false)
		{
		    osg::StateSet* stateset = modelNode->getOrCreateStateSet();
		    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
		}
		if(true)
		{
		    osg::StateSet * stateset = modelNode->getOrCreateStateSet();
		    osg::CullFace * cf=new osg::CullFace();
		    cf->setMode(osg::CullFace::BACK);
		    stateset->setAttributeAndModes( cf, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
		}
                if(true)
		{
		TextureResizeNonPowerOfTwoHintVisitor tr2v(false);
		modelNode->accept(tr2v);
                }
                if(false)
                {
                    StateSet* ss = modelNode->getOrCreateStateSet();
                    ss->setMode(GL_LIGHTING, StateAttribute::ON | osg::StateAttribute::OVERRIDE);
                    Material* mat = new Material();
                    mat->setColorMode(Material::AMBIENT_AND_DIFFUSE);
                    Vec4 color_dif(1, 1, 1, 1);
                    mat->setDiffuse(Material::FRONT_AND_BACK, color_dif);
                    ss->setAttribute(mat);
                    ss->setAttributeAndModes(mat, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
                }

//Add to SceneObject
  //   _query[q]->artifacts[inc]->model->name = basket;
    
float currentScale = _models3d[i]->scale;

	    SceneObject * so;
	    so = new SceneObject(name, false, false, false, true, false);
	    osg::Switch* switchNode = new osg::Switch();
	    so->addChild(switchNode);
	    PluginHelper::registerSceneObject(so,"Test");
	    so->attachToScene();
//Add currentNode to switchNode
      _models3d[i]->currentModelNode = modelNode;  
	switchNode->addChild(modelNode);
      _models3d[i]->switchNode = switchNode;

     //_root->addChild(modelNode);
//Add menu system
	    so->setNavigationOn(true);
	    so->setMovable(false);
	    so->addMoveMenuItem();
	    so->addNavigationMenuItem();
            float min = 0.0001;
            float max = 1;
            so->addScaleMenuItem("Scale",min,max,currentScale);
	    SubMenu * sm = new SubMenu("Position");
	    so->addMenuItem(sm);

	    MenuButton * mb;
	    mb = new MenuButton("Load");
	    mb->setCallback(this);
	    sm->addItem(mb);

	    SubMenu * savemenu = new SubMenu("Save");
	    sm->addItem(savemenu);

	    mb = new MenuButton("Save");
	    mb->setCallback(this);
	    savemenu->addItem(mb);
            _models3d[i]->saveMap = mb;

	    mb = new MenuButton("Save New Kml");
	    mb->setCallback(this);
	    savemenu->addItem(mb);
            _models3d[i]->saveNewMap = mb;

	    mb = new MenuButton("Reset to Origin");
	    mb->setCallback(this);
	    so->addMenuItem(mb);
            _models3d[i]->resetMap = mb;

            MenuCheckbox * mc;
	    mc = new MenuCheckbox("Active",false);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            _models3d[i]->activeMap = mc;

            
	    mc = new MenuCheckbox("Visible",true);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            _models3d[i]->visibleMap = mc;
            _models3d[i]->visible = true;

            float rValue = 0;
            min = -1;
            max = 1;
            MenuRangeValue* rt = new MenuRangeValue("rx",min,max,rValue);
            rt->setCallback(this);
	    so->addMenuItem(rt);
            _models3d[i]->rxMap = rt;

            rt = new MenuRangeValue("ry",min,max,rValue);
            rt->setCallback(this);
	    so->addMenuItem(rt);
            _models3d[i]->ryMap = rt;

            rt = new MenuRangeValue("rz",min,max,rValue);
            rt->setCallback(this);
	    so->addMenuItem(rt);
            _models3d[i]->rzMap = rt;
/*
	    mc = new MenuCheckbox("Panel Visible",true);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
 //           _query[q]->artifacts[inc]->model->pVisibleMap = mc;
           // _query[q]->artifacts[inc]->model->pVisible = true;
*/
Vec3 orig = currentPos; 
cerr << "Pos: " << orig.x() << " " << orig.y() << " " << orig.z() << "\n";

 so->setPosition(currentPos);     
 so->setScale(currentScale);
 so->setRotation(currentRot);     



    _models3d[i]->so = so;
    _models3d[i]->pos = so->getPosition();
    _models3d[i]->rot = so->getRotation();
    _models3d[i]->active = false;
    _models3d[i]->loaded = true;


    
}
void ArtifactVis2::parsePCXml(bool useHandPos)
{
    int index;
     bool addNewMod = false;
    if(newFileAvailable)
    {
      addNewMod = true;
    }
    if(newSelectedFile == "") return;
    newFileAvailable = false;
cerr << "Triggered\n";
    Matrix handMat = getHandToObjectMatrix();
    Vec3 pos = handMat.getTrans();

    string file;
    string filepath = newSelectedFile;
    string filename = getFileFromFilePath(filepath);
    size_t found=newSelectedFile.find(".");
    string filetype;
            if (found!=string::npos)
	    {
                 int start = int(found);
                 filetype = newSelectedFile;
                 filetype.erase(0,(start+1)); 
                 file = newSelectedFile;
                 file.erase((start+1),4);
                 file.append("kml");                
                 //cout <<" type: " << file << endl;
            }
string type = getKmlArray(file);
if(type == "Model")
{
   index = _models3d.size() -1;
   MenuCheckbox* site = new MenuCheckbox(_models3d[index]->name,true);
   site->setCallback(this);
   _showModelCB.push_back(site);
   string group = _models3d[index]->group; 
   addToModelDisplayMenu(group, site);
   if(useHandPos)
   {
   _models3d[index]->pos = pos; 
   }
}
else if(type == "PointCloud")
{
   index = _pointClouds.size() -1;
   MenuCheckbox* site = new MenuCheckbox(_pointClouds[index]->name,true);
   site->setCallback(this);
   _showPointCloudCB.push_back(site);
   string group = _pointClouds[index]->group; 
   addToPcDisplayMenu(group, site);
   if(useHandPos)
   {
   _pointClouds[index]->pos = pos; 
   }
}
else
 {
	Model* newModel = new Model();
	newModel->name = newSelectedName;
	newModel->filename = newSelectedFile;
	newModel->loaded = false;
	_pointClouds.push_back(newModel);
        index = _pointClouds.size() -1;
	MenuCheckbox* site = new MenuCheckbox(newSelectedName,true);
	site->setCallback(this);
	_showPointCloudCB.push_back(site);
    //Get Group
    found=filepath.find_last_of("/");
    string group;
            if (found!=string::npos)
	    {
                 int start = int(found);
                 group = filepath;
                 group.erase(start,(group.length()-start)); 
                 found=group.find_last_of("/");
                 if (found!=string::npos)
                 {
                   start = int(found);
                   group.erase(0,(start+1));
                  // cerr << "group: " << group << endl;
		 }             
                 //cout <<" type: " << file << endl;
            }
  // addToModelDisplayMenu(group, site);
   //Generate generic attributes
        Quat rot = osg::Quat(0, osg::Vec3d(1,0,0),0, osg::Vec3d(0,1,0),0, osg::Vec3d(0,0,1));
        float scale = 9; 
  //Fill Struct
    _pointClouds[index]->name = newSelectedName;
    _pointClouds[index]->filename = filename;
    _pointClouds[index]->group = group;
    _pointClouds[index]->filetype = filetype;
    _pointClouds[index]->fullpath = filepath;
    _pointClouds[index]->modelType = "PointCloud";
    _pointClouds[index]->pos = pos; 
    _pointClouds[index]->rot = rot;
    _pointClouds[index]->origPos = pos; 
    _pointClouds[index]->origRot = rot;
    _pointClouds[index]->scale = scale; 
    _pointClouds[index]->origScale = scale; 
 }
    
addNewPC(index);

 
 newSelectedFile = "";
 newSelectedName = "";
}
void ArtifactVis2::parseModelXml(bool useHandPos)
{
    int index;
     bool addNewMod = false;
    if(newFileAvailable)
    {
      addNewMod = true;
    }
    if(newSelectedFile == "") return;
    newFileAvailable = false;
cerr << "Triggered\n";
    Matrix handMat = getHandToObjectMatrix();
    Vec3 pos = handMat.getTrans();

    string file;
    string filepath = newSelectedFile;
    string filename = getFileFromFilePath(filepath);
    size_t found=newSelectedFile.find(".");
    string filetype;
            if (found!=string::npos)
	    {
                 int start = int(found);
                 filetype = newSelectedFile;
                 filetype.erase(0,(start+1)); 
                 file = newSelectedFile;
                 file.erase((start+1),4);
                 file.append("kml");                
                 //cout <<" type: " << file << endl;
            }
string type = getKmlArray(file);
if(type == "Model")
{
   index = _models3d.size() -1;
   MenuCheckbox* site = new MenuCheckbox(_models3d[index]->name,true);
   site->setCallback(this);
   _showModelCB.push_back(site);
   string group = _models3d[index]->group; 
   addToModelDisplayMenu(group, site);
   if(useHandPos)
   {
   _models3d[index]->pos = pos; 
   }
}
else if(type == "PointCloud")
{
   index = _pointClouds.size() -1;
   MenuCheckbox* site = new MenuCheckbox(_pointClouds[index]->name,true);
   site->setCallback(this);
   _showPointCloudCB.push_back(site);
   string group = _pointClouds[index]->group; 
   addToPcDisplayMenu(group, site);
   if(useHandPos)
   {
   _pointClouds[index]->pos = pos; 
   }
}
 else
 {
	Model* newModel = new Model();
	newModel->name = newSelectedName;
	newModel->filename = newSelectedFile;
	newModel->loaded = false;
	_models3d.push_back(newModel);
        index = _models3d.size() -1;
	MenuCheckbox* site = new MenuCheckbox(newSelectedName,true);
	site->setCallback(this);
	_showModelCB.push_back(site);
    //Get Group
    found=filepath.find_last_of("/");
    string group;
            if (found!=string::npos)
	    {
                 int start = int(found);
                 group = filepath;
                 group.erase(start,(group.length()-start)); 
                 found=group.find_last_of("/");
                 if (found!=string::npos)
                 {
                   start = int(found);
                   group.erase(0,(start+1));
                  // cerr << "group: " << group << endl;
		 }             
                 //cout <<" type: " << file << endl;
            }
   addToModelDisplayMenu(group, site);
   //Generate generic attributes
        Quat rot = osg::Quat(0, osg::Vec3d(1,0,0),0, osg::Vec3d(0,1,0),0, osg::Vec3d(0,0,1));
        float scale = 0.001; 
  //Fill Struct
    _models3d[index]->name = newSelectedName;
    _models3d[index]->filename = filename;
    _models3d[index]->group = group;
    _models3d[index]->filetype = filetype;
    _models3d[index]->fullpath = filepath;
    _models3d[index]->modelType = "Model";
    _models3d[index]->pos = pos; 
    _models3d[index]->rot = rot;
    _models3d[index]->origPos = pos; 
    _models3d[index]->origRot = rot;
    _models3d[index]->scale = scale; 
    _models3d[index]->origScale = scale; 
 }
    
addNewModel(index);

 
 newSelectedFile = "";
 newSelectedName = "";
}
osg::Matrix ArtifactVis2::getHandToObjectMatrix()
{

           Matrix handMat0 = TrackingManager::instance()->getHandMat(0);
           osg::Vec3 viewerPoint = TrackingManager::instance()->getHeadMat(0).getTrans();
           osg::Matrixd w2o = PluginHelper::getWorldToObjectTransform();
            Matrix handMat;
                if(true)
                {

                   float   _distance = ConfigManager::getFloat("distance", "MenuSystem.BoardMenu.Position",2000.0);
                    osg::Vec3 menuPoint = osg::Vec3(0,2000,0);
                    menuPoint = menuPoint * handMat0;

                   // if(event->asMouseEvent())
                    if(false)
                    {
                        osg::Vec3 menuOffset = osg::Vec3(0,0,0);
                        osg::Matrix m;
                        m.makeTranslate(menuPoint);
                        handMat = m * w2o;
                    }
                    else
                    {

                        osg::Vec3 viewerDir = viewerPoint - menuPoint;
                        viewerDir.z() = 0.0;

                        osg::Matrix menuRot;
                        menuRot.makeRotate(osg::Vec3(0,-1,0),viewerDir);

                        osg::Vec3 menuOffset = osg::Vec3(0,0,0);
                        handMat = (osg::Matrix::translate(-menuOffset) * menuRot * osg::Matrix::translate(menuPoint)) * w2o;
                    }

                }
return handMat;
}
osg::Matrix ArtifactVis2::getHandToSceneMatrix()
{

           Matrix handMat0 = TrackingManager::instance()->getHandMat(0);
           osg::Vec3 viewerPoint = TrackingManager::instance()->getHeadMat(0).getTrans();
//           osg::Matrixd w2o = PluginHelper::getWorldToObjectTransform();
            Matrix handMat;
                if(true)
                {

                   float   _distance = ConfigManager::getFloat("distance", "MenuSystem.BoardMenu.Position",2000.0);
                    osg::Vec3 menuPoint = osg::Vec3(0,2000,0);
                    menuPoint = menuPoint * handMat0;

                   // if(event->asMouseEvent())
                    if(false)
                    {
                        menuPoint = osg::Vec3(0,1405,0);
                        menuPoint = menuPoint * handMat0;
                        osg::Vec3 menuOffset = osg::Vec3(0,0,0);
                        osg::Matrix m;
                        m.makeTranslate(menuPoint);
                        handMat = m;
                    }
                    else
                    {

                        osg::Vec3 viewerDir = viewerPoint - menuPoint;
                        viewerDir.z() = 0.0;

                        osg::Matrix menuRot;
                        menuRot.makeRotate(osg::Vec3(0,-1,0),viewerDir);

                        osg::Vec3 menuOffset = osg::Vec3(0,0,0);
                        handMat = (osg::Matrix::translate(-menuOffset) * menuRot * osg::Matrix::translate(menuPoint));
                    }

                }
return handMat;
}
void ArtifactVis2::startLineObject()
{
    //get handpos
   Matrix handMat = getHandToObjectMatrix(); 
   Vec3 currentPos = handMat.getTrans();
       //Vec3 scenePos = getHandToSceneMatrix().getTrans();
       Vec3 scenePos = currentPos;
    //Setup Colors

    Vec4f color = Vec4f(0, 0.42, 0.92, 1);
    Vec4f colorR = Vec4f(0.92, 0, 0, 1);
    Vec4f colorG = Vec4f(0, 0.92, 0, 1);
    //Setup Initial Pos

    osg::Vec3 pos;
    pos = Vec3(0,0,0);
    //New LineGroup

    LineGroup* lineGroup = new LineGroup();

    lineGroup->scenePos = scenePos;
    //make First cube geode
    Sphere* cubeShape = new Sphere(pos, _vertexRadius);
    ShapeDrawable* shapeDrawable = new ShapeDrawable(cubeShape);
   // shapeDrawable->setTessellationHints(hints);
    shapeDrawable->setColor(colorR);
    osg::Geode* sphereGeode = new Geode();  
    sphereGeode->addDrawable(shapeDrawable);

    lineGroup->cubeGeode.push_back(sphereGeode);
    lineGroup->cubeShape.push_back(cubeShape);
    lineGroup->vertex.push_back(pos);

    //make Second cube geode
    Sphere*  cubeShape2 = new Sphere(pos, _vertexRadius);
    lineGroup->cubeShape.push_back(cubeShape2);
    ShapeDrawable* shapeDrawable2 = new ShapeDrawable(lineGroup->cubeShape[1]);
   // shapeDrawable2->setTessellationHints(hints);
    shapeDrawable2->setColor(colorG);
    osg::Geode* sphereGeode2 = new Geode();  
    sphereGeode2->addDrawable(shapeDrawable2);
    lineGroup->cubeGeode.push_back(sphereGeode2);
//    lineGroup->cubeShape.push_back(cubeShape2);
    lineGroup->vertex.push_back(pos);

    //make  line geode
    osg:Geometry* connector = new osg::Geometry();
    osg::Vec3Array* verts = new osg::Vec3Array();
    verts->push_back(pos);
    verts->push_back(pos);

    connector->setVertexArray(verts);

    osg::DrawElementsUInt* ele = new osg::DrawElementsUInt(
            osg::PrimitiveSet::LINES,0);

    ele->push_back(0);
    ele->push_back(1);
    connector->addPrimitiveSet(ele);

    osg::Vec4Array* colors = new osg::Vec4Array;
    colors->push_back(color);

    osg::TemplateIndexArray<unsigned int,osg::Array::UIntArrayType,4,4> *colorIndexArray;
    colorIndexArray = new osg::TemplateIndexArray<unsigned int,
            osg::Array::UIntArrayType,4,4>;
    colorIndexArray->push_back(0);
    colorIndexArray->push_back(0);
   
   connector->setColorArray(colors);
   connector->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    Geode* connectorGeode = new Geode();

            StateSet* state3(connectorGeode->getOrCreateStateSet());

            osg::LineWidth* linewidth = new osg::LineWidth();
            linewidth->setWidth(2.0f); 
            state3->setAttributeAndModes(linewidth, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
            connectorGeode->setStateSet(state3);




    lineGroup->connector.push_back(connector);
    connectorGeode->addDrawable(connector);


//Create Text Drawable 1

        osgText::Text* label = new osgText::Text();
        label->setText("0");
        label->setUseDisplayList(false);
        label->setAxisAlignment(osgText::Text::SCREEN);
        label->setPosition(pos + Vec3f(0, 0, _vertexRadius * 1.1));
        label->setAlignment(osgText::Text::CENTER_CENTER);
        label->setCharacterSize(15);
        label->setCharacterSizeMode(osgText::Text::SCREEN_COORDS);
        lineGroup->label.push_back(label);
        Geode* textGeode = new Geode();
        textGeode->addDrawable(label);

//Create Text Drawable 2

        osgText::Text* label2 = new osgText::Text();
        label2->setText("0");
        label2->setUseDisplayList(false);
        label2->setAxisAlignment(osgText::Text::SCREEN);
        label2->setPosition(pos + Vec3f(0, 0, _vertexRadius * 1.1));
        label2->setAlignment(osgText::Text::CENTER_CENTER);
        label2->setCharacterSize(15);
        label2->setCharacterSizeMode(osgText::Text::SCREEN_COORDS);
        lineGroup->label.push_back(label2);
        Geode* textGeode2 = new Geode();
        textGeode2->addDrawable(label2);


  lineGroup->distanceTotal = 0;
  lineGroup->distance.push_back(0);
  lineGroup->distance.push_back(0);
//Create sceneobject

    string name = "start";
	    SceneObject * so;
	    so = new SceneObject(name, false, false, false, true, false);
	    osg::Switch* switchNode = new osg::Switch();
	    so->addChild(switchNode);
	    PluginHelper::registerSceneObject(so,"Test");
	    so->attachToScene();
//Add currentNode to switchNode
	switchNode->addChild(lineGroup->cubeGeode[0]);
	switchNode->addChild(lineGroup->cubeGeode[1]);
	switchNode->addChild(connectorGeode);
	switchNode->addChild(textGeode);
	switchNode->addChild(textGeode2);
        lineGroup->switchNode = switchNode;


//Add menu system
	    so->setNavigationOn(true);
	    so->setMovable(false);
	    so->addMoveMenuItem();
	    so->addNavigationMenuItem();

	    SubMenu * sm = new SubMenu("Position");
	    so->addMenuItem(sm);

	    MenuButton * mb;
	    mb = new MenuButton("Load");
	    mb->setCallback(this);
	    sm->addItem(mb);

	    SubMenu * savemenu = new SubMenu("Save");
	    sm->addItem(savemenu);

	    mb = new MenuButton("Save");
	    mb->setCallback(this);
	    savemenu->addItem(mb);
            lineGroup->saveMap = mb;
/*
	    mb = new MenuButton("Reset to Origin");
	    mb->setCallback(this);
	    so->addMenuItem(mb);
            lineGroup->resetMap = mb;
*/
	    mb = new MenuButton("Delete");
	    mb->setCallback(this);
	    so->addMenuItem(mb);
            lineGroup->deleteMap = mb;

            MenuCheckbox * mc;
	    mc = new MenuCheckbox("Active",false);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            lineGroup->activeMap = mc;

	    mc = new MenuCheckbox("Editing",true);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            lineGroup->editingMap = mc;
            
	    mc = new MenuCheckbox("Visible",true);
	    mc->setCallback(this);
	    so->addMenuItem(mc);
            lineGroup->visibleMap = mc;
            lineGroup->visible = true;


float currentScale = 1;
Vec3 orig = scenePos; 
cerr << "Pos: " << orig.x() << " " << orig.y() << " " << orig.z() << "\n";

 so->setPosition(currentPos);     
 so->setScale(currentScale);
// so->setRotation(currentRot);     



    lineGroup->so = so;
    lineGroup->pos = so->getPosition();
    lineGroup->rot = so->getRotation();
    lineGroup->open = true;   
    lineGroup->active = false;    
    lineGroup->editing = true;    
    lineGroupsEditing = true;
    _lineGroups.push_back(lineGroup); 

     //addtracker


}

void ArtifactVis2::closeLineVertex(int i)
{
                 _lineGroups[i]->editing = false;
                 lineGroupsEditing = false;
                 _lineGroups[i]->open = false;   

       int lEndIndex = _lineGroups[i]->vertex.size() - 1;
       int lStartIndex = lEndIndex -1;
       
    Vec4f color = Vec4f(0, 0.42, 0.92, 1);
    Vec4f colorT = Vec4f(0, 0.42, 0.92, 0.5);

    //make  line geode
    osg:Geometry* connector = new osg::Geometry();
    osg::Vec3Array* verts = new osg::Vec3Array();
    		verts->push_back(_lineGroups[i]->vertex[lEndIndex]);
    		verts->push_back(_lineGroups[i]->vertex[0]);

    connector->setVertexArray(verts);

    osg::DrawElementsUInt* ele = new osg::DrawElementsUInt(
            osg::PrimitiveSet::LINES,0);

    ele->push_back(0);
    ele->push_back(1);
    connector->addPrimitiveSet(ele);

    osg::Vec4Array* colors = new osg::Vec4Array;
    colors->push_back(color);

    osg::TemplateIndexArray<unsigned int,osg::Array::UIntArrayType,4,4> *colorIndexArray;
    colorIndexArray = new osg::TemplateIndexArray<unsigned int,
            osg::Array::UIntArrayType,4,4>;
    colorIndexArray->push_back(0);
    colorIndexArray->push_back(0);
   
   connector->setColorArray(colors);
   connector->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    Geode* connectorGeode = new Geode();

            StateSet* state3(connectorGeode->getOrCreateStateSet());

            osg::LineWidth* linewidth = new osg::LineWidth();
            linewidth->setWidth(2.0f); 
            state3->setAttributeAndModes(linewidth, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
            connectorGeode->setStateSet(state3);




    _lineGroups[i]->connector.push_back(connector);
    connectorGeode->addDrawable(connector);
//Update Label Total

       float distance = (_lineGroups[i]->vertex[lEndIndex] - _lineGroups[i]->vertex[0]).length();
        
       _lineGroups[i]->distanceTotal += distance;
       stringstream ss;
       ss << "Perimeter:" << _lineGroups[i]->distanceTotal << "m";
            string distText = ss.str();
        _lineGroups[i]->label[0]->setText(distText);




//Make PolygonGeode
            Vec3Array* coords = new Vec3Array();
            for (int n = 0; n <_lineGroups[i]->vertex.size(); n++)
            {
              coords->push_back(_lineGroups[i]->vertex[n]);
            }
            //Add Bottom
            float depthX = 0;
            float depthY = 0;
            float depthZ = 0;
            for (int n = 0; n <_lineGroups[i]->vertex.size(); n++)
            {
              coords->push_back(_lineGroups[i]->vertex[n] + Vec3(depthX,depthY,depthZ));
            }
            
            int size = coords->size() / 2;

            Geometry* geom = new Geometry();
            Geometry* tgeom = new Geometry();
            Geode* fgeode = new Geode();
            Geode* lgeode = new Geode();
            geom->setVertexArray(coords);
            tgeom->setVertexArray(coords);

            for (int n = 0; n < size; n++)
            {
                DrawElementsUInt* face = new DrawElementsUInt(PrimitiveSet::QUADS, 0);
                face->push_back(n);
                face->push_back(n + size);
                face->push_back(((n + 1) % size) + size);
                face->push_back((n + 1) % size);
                geom->addPrimitiveSet(face);

                if (n < size - 1) //Commented out for now, adds caps to the polyhedra.
                {
                    face = new DrawElementsUInt(PrimitiveSet::TRIANGLES, 0);
                    face->push_back(0);
                    face->push_back(n);
                    face->push_back(n + 1);
                    geom->addPrimitiveSet(face);
                    tgeom->addPrimitiveSet(face);
                    face = new DrawElementsUInt(PrimitiveSet::TRIANGLES, 0);
                    face->push_back(size);
                    face->push_back(size + n);
                    face->push_back(size + n + 1);
                    geom->addPrimitiveSet(face);
                    //tgeom->addPrimitiveSet(face);
                }
            }


            StateSet* state(fgeode->getOrCreateStateSet());
            Material* mat(new Material);
            mat->setColorMode(Material::DIFFUSE);
            mat->setDiffuse(Material::FRONT_AND_BACK, colorT);
            state->setAttribute(mat);
            state->setRenderingHint(StateSet::TRANSPARENT_BIN);
            state->setMode(GL_BLEND, StateAttribute::ON);
            state->setMode(GL_LIGHTING, StateAttribute::OFF);
            osg::PolygonMode* polymode = new osg::PolygonMode;
            polymode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
            state->setAttributeAndModes(polymode, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
            fgeode->setStateSet(state);
            fgeode->addDrawable(geom);
            StateSet* state2(lgeode->getOrCreateStateSet());
            Material* mat2(new Material);
            state2->setRenderingHint(StateSet::OPAQUE_BIN);
            mat2->setColorMode(Material::DIFFUSE);
            mat2->setDiffuse(Material::FRONT_AND_BACK, color);
            state2->setAttribute(mat2);
            state->setMode(GL_BLEND, StateAttribute::ON);
            state2->setMode(GL_LIGHTING, StateAttribute::OFF);
            osg::PolygonMode* polymode2 = new osg::PolygonMode;
            polymode2->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
            state2->setAttributeAndModes(polymode2, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
            lgeode->setStateSet(state2);
            lgeode->addDrawable(geom);
//Add geodes to switchNode

_lineGroups[i]->switchNode->addChild(connectorGeode);
_lineGroups[i]->switchNode->addChild(fgeode);
if(false)
{
_lineGroups[i]->switchNode->addChild(lgeode);
}
}
void ArtifactVis2::addLineVertex(int i)
{
       //Vec3 pos = getHandToObjectMatrix().getTrans();
       Vec3 pos = getHandToSceneMatrix().getTrans();
       Vec3 orig = _lineGroups[i]->so->getPosition();
       //pos = pos + orig;
       int lEndIndex = _lineGroups[i]->vertex.size();
       int lStartIndex = lEndIndex -1;
       int lineIndex = _lineGroups[i]->connector.size();

         //Update Line
    Vec4f color = Vec4f(0, 0.42, 0.92, 1);
    Vec4f colorG = Vec4f(0, 0.92, 0, 1);
    //make First cube geode
    Sphere* cubeShape = new Sphere(pos, _vertexRadius);
    ShapeDrawable* shapeDrawable = new ShapeDrawable(cubeShape);
   // shapeDrawable->setTessellationHints(hints);
    shapeDrawable->setColor(colorG);
    osg::Geode* sphereGeode = new Geode();  
    sphereGeode->addDrawable(shapeDrawable);

    _lineGroups[i]->cubeGeode[lStartIndex] = sphereGeode;
    _lineGroups[i]->cubeShape[lStartIndex] = cubeShape;
    _lineGroups[i]->vertex[lStartIndex] =pos;

    //make Second cube geode
    Sphere*  cubeShape2 = new Sphere(pos, _vertexRadius);
    ShapeDrawable* shapeDrawable2 = new ShapeDrawable(cubeShape2);
   // shapeDrawable2->setTessellationHints(hints);
    shapeDrawable2->setColor(colorG);
    osg::Geode* sphereGeode2 = new Geode();  
    sphereGeode2->addDrawable(shapeDrawable2);
    _lineGroups[i]->cubeGeode.push_back(sphereGeode2);
    _lineGroups[i]->cubeShape.push_back(cubeShape2);
    _lineGroups[i]->vertex.push_back(pos);

    //make  line geode
    osg:Geometry* connector = new osg::Geometry();
    osg::Vec3Array* verts = new osg::Vec3Array();
    verts->push_back(pos);
    verts->push_back(pos);

    connector->setVertexArray(verts);

    osg::DrawElementsUInt* ele = new osg::DrawElementsUInt(
            osg::PrimitiveSet::LINES,0);

    ele->push_back(0);
    ele->push_back(1);
    connector->addPrimitiveSet(ele);

    osg::Vec4Array* colors = new osg::Vec4Array;
    colors->push_back(color);

    osg::TemplateIndexArray<unsigned int,osg::Array::UIntArrayType,4,4> *colorIndexArray;
    colorIndexArray = new osg::TemplateIndexArray<unsigned int,
            osg::Array::UIntArrayType,4,4>;
    colorIndexArray->push_back(0);
    colorIndexArray->push_back(0);
   
   connector->setColorArray(colors);
   connector->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    Geode* connectorGeode = new Geode();

            StateSet* state3(connectorGeode->getOrCreateStateSet());

            osg::LineWidth* linewidth = new osg::LineWidth();
            linewidth->setWidth(2.0f); 
            state3->setAttributeAndModes(linewidth, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
            connectorGeode->setStateSet(state3);




    _lineGroups[i]->connector.push_back(connector);
    connectorGeode->addDrawable(connector);

//Store Last Text Drawable 
         
       float distance = (pos - _lineGroups[i]->vertex[lStartIndex - 1]).length();
        
       _lineGroups[i]->distanceTotal += distance;
       stringstream ss;
       ss << distance << "m";
            string distText = ss.str();
        _lineGroups[i]->label[lStartIndex]->setText(distText);
        _lineGroups[i]->label[lStartIndex]->setUseDisplayList(false);
        _lineGroups[i]->label[lStartIndex]->setAxisAlignment(osgText::Text::SCREEN);
        _lineGroups[i]->label[lStartIndex]->setPosition(pos + Vec3f(0, 0, _vertexRadius * 1.1));
        _lineGroups[i]->label[lStartIndex]->setAlignment(osgText::Text::CENTER_CENTER);
        _lineGroups[i]->label[lStartIndex]->setCharacterSize(15);
        _lineGroups[i]->label[lStartIndex]->setCharacterSizeMode(osgText::Text::SCREEN_COORDS);
//Add new Text Geode


        osgText::Text* label = new osgText::Text();
        label->setText("0");
        label->setUseDisplayList(false);
        label->setAxisAlignment(osgText::Text::SCREEN);
        label->setPosition(pos + Vec3f(0, 0, _vertexRadius * 1.1));
        label->setAlignment(osgText::Text::CENTER_CENTER);
        label->setCharacterSize(15);
        label->setCharacterSizeMode(osgText::Text::SCREEN_COORDS);
        _lineGroups[i]->label.push_back(label);
        Geode* textGeode = new Geode();
        textGeode->addDrawable(label);


 _lineGroups[i]->distance.push_back(0);
//Add geodes to switchNode

_lineGroups[i]->switchNode->removeChild(_lineGroups[i]->cubeGeode[lStartIndex]);
_lineGroups[i]->switchNode->addChild(_lineGroups[i]->cubeGeode[lStartIndex]);
_lineGroups[i]->switchNode->addChild(_lineGroups[i]->cubeGeode[lEndIndex]);
_lineGroups[i]->switchNode->addChild(connectorGeode);
_lineGroups[i]->switchNode->addChild(textGeode);

                 _lineGroups[i]->editing = true;
                 lineGroupsEditing = true;   


}

void ArtifactVis2::updateLineGroup()
{
  int i;

if(lineGroupsEditing)
{
  if(_lineGroups.size() > 0)
  {
    for (i = 0; i < _lineGroups.size(); i++)
    {
    if(_lineGroups[i]->editing)
    {

       
           osg::Matrixd o2w = PluginHelper::getObjectToWorldTransform();
        
       Matrix poMat = getHandToObjectMatrix();
       Vec3 pos;
       
       osg::Matrix soMat = _lineGroups[i]->so->getObjectToWorldMatrix();
       Vec3 origSo = _lineGroups[i]->so->getPosition();
      // poMat = poMat * soMat;
      
       pos = poMat.getTrans() - origSo;
      // pos = ;
//bangdist
       int lEndIndex = _lineGroups[i]->vertex.size() - 1;
       int lStartIndex = lEndIndex -1;
       int lineIndex = lStartIndex;

      // Vec3 viewOrig = PluginHelper::getObjectTransform().getMatrix().getTrans(); 
      // Vec3 viewOrig = PluginHelper::getHeadMat(0).getTrans() * w2o; 
//cerr << "Pos: " << orig.x() << " " << orig.y() << " " << orig.z() << "\n";
      // viewOrig = viewOrig - orig;
      // pos = pos - orig + viewOrig;
       
       if(_lineGroups[i]->vertex[lEndIndex] != pos)
       {
          _lineGroups[i]->vertex[lEndIndex] = pos;

       float distance = (pos - _lineGroups[i]->vertex[lStartIndex]).length();
        _lineGroups[i]->distance[lEndIndex] = distance;

       float distanceCrowFly = _lineGroups[i]->distanceTotal + distance;
       
       stringstream ss;
       ss << distance << "m" << " (Total:" << distanceCrowFly << ")";
            string distText = ss.str();
         //Update Line
         //...
                osg::Vec3Array* verts = new osg::Vec3Array();
    		verts->push_back(_lineGroups[i]->vertex[lStartIndex]);
    		verts->push_back(_lineGroups[i]->vertex[lEndIndex]);

    Vec4f color = Vec4f(0, 0.42, 0.92, 1);
    Vec4f colorG = Vec4f(0, 0.92, 0, 1);
    Sphere*  cubeShape2 = new Sphere(pos, _vertexRadius);
    ShapeDrawable* shapeDrawable2 = new ShapeDrawable(cubeShape2);
  //  shapeDrawable2->setTessellationHints(hints);
    shapeDrawable2->setColor(colorG);
    osg::Geode* sphereGeode2 = new Geode();  
    sphereGeode2->addDrawable(shapeDrawable2);



//Create Text Drawable Update

      //  osgText::Text* label = new osgText::Text();
        _lineGroups[i]->label[lEndIndex]->setText(distText);
        _lineGroups[i]->label[lEndIndex]->setUseDisplayList(false);
        _lineGroups[i]->label[lEndIndex]->setAxisAlignment(osgText::Text::SCREEN);
        _lineGroups[i]->label[lEndIndex]->setPosition(pos + Vec3f(0, 0, _vertexRadius * 1.1));
        _lineGroups[i]->label[lEndIndex]->setAlignment(osgText::Text::CENTER_CENTER);
        _lineGroups[i]->label[lEndIndex]->setCharacterSize(15);
        _lineGroups[i]->label[lEndIndex]->setCharacterSizeMode(osgText::Text::SCREEN_COORDS);
/*
        lineGroup->label.push_back(label);
        Geode* textGeode = new Geode();
        textGeode->addDrawable(label);
*/



Vec3 orig = pos;
//cerr << "Pos: " << orig.x() << " " << orig.y() << " " << orig.z() << "\n";
_lineGroups[i]->switchNode->removeChild(_lineGroups[i]->cubeGeode[lEndIndex]);
   _lineGroups[i]->cubeGeode[lEndIndex] = sphereGeode2;
    _lineGroups[i]->cubeShape[lEndIndex] = cubeShape2;
_lineGroups[i]->switchNode->addChild(_lineGroups[i]->cubeGeode[lEndIndex]);

                 _lineGroups[i]->connector[lineIndex]->setVertexArray(verts);
                 _lineGroups[i]->editing = true;
       }
    break;
    }
    }
 }
}
} 
void ArtifactVis2::saveModelConfig(Model* saveModel, bool newConfig)
{
    string name = saveModel->name;
    string path = getPathFromFilePath(saveModel->fullpath);
    string filename = saveModel->filename;
    size_t found=name.find(".");
    string filetype;
    string file;
            if (found!=string::npos)
	    {
                 int start = int(found);
                 filetype = name;
                 filetype.erase(0,(start+1)); 
                 if(filetype == "kml")
                 {
                  filetype = saveModel->filetype;
                  file = path;
                  file.append(name);
                 }
                 else
                 {
                 filename = name;
                 name.erase((start+1),4);
                 name.append("kml");
                 file = path;
                 file.append(name);
                 }                

	     }
if(newConfig)
{
  if(manualEnterName != "")
  {
     name = manualEnterName;
  }
     string newFile;
     bool nameExists = true;
     name.erase((name.length()-4),4);
     cerr << "Name : " << name << "Path: " << path << endl;
     string tempName = "";
     int inc = 0;
     while(nameExists)
     {          
                 tempName = name;
                 std:stringstream ss;
                 ss << inc;
                 tempName.append("_");
                 tempName.append(ss.str());
                 tempName.append(".kml");
                 newFile = path;
                 newFile.append(tempName);
                 inc++;
       
           if(!modelExists(newFile.c_str())) nameExists = false;
     }
     name = tempName;
     file = newFile;

}


//Create Placemarks
Vec3 pos = saveModel->so->getPosition();
Quat rot = saveModel->so->getRotation();

//TODO:Convert Quat to Euler
//Matrix rMat = _models3d[i]->so->getTransform();
//Vec3 rot = matrix_to_euler(rMat); 

float scaleFloat = saveModel->so->getScale();
string q_type = saveModel->modelType;
string q_group = saveModel->group;

cerr << "NewFile: " << file << endl;
saveTo3Dkml(name, filename, file, filetype, pos, rot, scaleFloat, q_type, q_group);
}


void ArtifactVis2::saveTo3Dkml(string name,string filename, string file, string filetype, Vec3 pos, Quat rot, float scaleFloat,string q_type, string q_group) 
{

    mxml_node_t *xml;    /* <?xml ... ?> */
    mxml_node_t *kml;   /* <kml> */
    mxml_node_t *document;   /* <Document> */
    mxml_node_t *nameKML;   /* <name> */
    mxml_node_t *filetypeKML;   /* <name> */
    mxml_node_t *open;   /* <name> */
    mxml_node_t *type;   /* <type> */
    mxml_node_t *timestamp;   /* <timestamp> */

    mxml_node_t *placemark;   /* <Placemark> */
    mxml_node_t *description;   /* <description> */

    mxml_node_t *lookat;   /* <LookAt> */
    mxml_node_t *longitude;   /* <data> */
    mxml_node_t *latitude;   /* <data> */
    mxml_node_t *altitude;   /* <data> */
    mxml_node_t *range;   /* <data> */
    mxml_node_t *tilt;   /* <data> */
    mxml_node_t *heading;   /* <data> */
    mxml_node_t *w;   /* <data> */
    mxml_node_t *styleurl;   /* <data> */
    mxml_node_t *altitudeMode;   /* <data> */
    mxml_node_t *group;   /* <data> */
    mxml_node_t *model;   /* <data> */
    mxml_node_t *orientation;   /* <data> */
    mxml_node_t *scale;   /* <data> */
    mxml_node_t *x;   /* <data> */
    mxml_node_t *y;   /* <data> */
    mxml_node_t *z;   /* <data> */
    mxml_node_t *link;   /* <data> */
    mxml_node_t *href;   /* <data> */
    mxml_node_t *resourceMap;   /* <data> */

//Create KML Container

//KML Name
    string q_name = name;
   // string g_timestamp = getTimeStamp();
    string g_timestamp = "00";

   const char* kml_name = q_name.c_str();
   const char* kml_timestamp = g_timestamp.c_str();

xml = mxmlNewXML("1.0");
        kml = mxmlNewElement(xml, "kml");
            document = mxmlNewElement(kml, "Document");
                nameKML = mxmlNewElement(document, "name");
                  mxmlNewText(nameKML, 0, kml_name);
                open = mxmlNewElement(document, "open");
                  mxmlNewText(open, 0, "1");
                timestamp = mxmlNewElement(document, "timestamp");
                  mxmlNewText(timestamp, 0, kml_timestamp);
//.................................................................
//Get Placemarks





   //Get Comments Description
   string q_description = "";

stringstream buffer;
   buffer << pos.x();
   string q_longitude = buffer.str();
   buffer.str("");
   buffer << pos.y();
   string q_latitude = buffer.str();
   buffer.str("");
   buffer << pos.z();
   string q_altitude = buffer.str();
   buffer.str("");
   buffer << rot.x();
   string q_x = buffer.str();
   buffer.str("");
   buffer << rot.y();
   string q_y = buffer.str();
   buffer.str("");
   buffer << rot.z();
   string q_z = buffer.str();
   buffer.str("");
   buffer << rot.w();
   string q_w = buffer.str();
   buffer.str("");
   buffer << scaleFloat;
   string scaleTemp = buffer.str();
   buffer.str("");
   string q_scaleX = scaleTemp;
   string q_scaleY = scaleTemp;
   string q_scaleZ = scaleTemp;

   string q_href = filename;

                placemark = mxmlNewElement(document, "Placemark");
                    nameKML = mxmlNewElement(placemark, "name");
                      mxmlNewText(nameKML, 0, q_name.c_str());
                    type = mxmlNewElement(placemark, "type");
                      mxmlNewText(type, 0, q_type.c_str());
                    filetypeKML = mxmlNewElement(placemark, "filetype");
                      mxmlNewText(filetypeKML, 0, filetype.c_str());
                    group = mxmlNewElement(placemark, "group");
                      mxmlNewText(group, 0, q_group.c_str());
                    styleurl = mxmlNewElement(placemark, "styleUrl");
                      mxmlNewText(styleurl, 0, "#msn_GR");

                    description = mxmlNewElement(placemark, "description");
                      mxmlNewText(description, 0, q_description.c_str());
                    model = mxmlNewElement(placemark, "Model");
                        altitudeMode = mxmlNewElement(model, "altitudeMode");
                          mxmlNewText(altitudeMode, 0, "absolute");
                    
                    lookat = mxmlNewElement(model, "Location");
                        longitude = mxmlNewElement(lookat, "longitude");
                          mxmlNewText(longitude, 0, q_longitude.c_str());
                        latitude = mxmlNewElement(lookat, "latitude");
                          mxmlNewText(latitude, 0, q_latitude.c_str());
                        altitude = mxmlNewElement(lookat, "altitude");
                          mxmlNewText(altitude, 0, q_altitude.c_str());
                    orientation = mxmlNewElement(model, "Orientation");
                        range = mxmlNewElement(orientation, "heading");
                          mxmlNewText(range, 0, q_x.c_str());
                        tilt = mxmlNewElement(orientation, "tilt");
                          mxmlNewText(tilt, 0, q_y.c_str());
                        heading = mxmlNewElement(orientation, "roll");
                          mxmlNewText(heading, 0, q_z.c_str());
                        w = mxmlNewElement(orientation, "w");
                          mxmlNewText(w, 0, q_w.c_str());
                    scale = mxmlNewElement(model, "Orientation");
                        x = mxmlNewElement(scale, "x");
                          mxmlNewText(x, 0, q_scaleX.c_str());
                        y = mxmlNewElement(scale, "y");
                          mxmlNewText(y, 0, q_scaleY.c_str());
                        z = mxmlNewElement(scale, "z");
                          mxmlNewText(z, 0, q_scaleZ.c_str());
                    link = mxmlNewElement(model, "Link");
                        href = mxmlNewElement(link, "href");
                          mxmlNewText(href, 0, q_href.c_str());
//.......................................................
//Save File

  const char *ptr;
    ptr = "";
  ptr = mxmlSaveAllocString(xml, MXML_NO_CALLBACK);
    //cout << ptr;
    FILE *fp;
    
    filename = file;
    kml_name = filename.c_str();
    fp = fopen(kml_name, "w");

    fprintf(fp, ptr);

    fclose(fp);
 

}
Vec3 ArtifactVis2::matrix_to_euler(osg::Matrix colMatrix)
{
  //Taken from Quat/Matrix.c
//Does not work
#define  Q_EPSILON   (1e-10)

   double sinPitch, cosPitch, sinRoll, cosRoll, sinYaw, cosYaw;


   sinPitch = -colMatrix(2,0);
   cosPitch = sqrt(1 - sinPitch*sinPitch);

   if ( fabs(cosPitch) > Q_EPSILON ) 
   {
      sinRoll = colMatrix(2,1) / cosPitch;
      cosRoll = colMatrix(2,2) / cosPitch;
      sinYaw = colMatrix(1,0) / cosPitch;
      cosYaw = colMatrix(0,0) / cosPitch;
   } 
   else 
   {
      sinRoll = -colMatrix(1,2);
      cosRoll = colMatrix(1,1);
      sinYaw = 0;
      cosYaw = 1;
   }
   Vec3 radians;
   Vec3 angles;
   radians.x() = atan2(sinYaw, cosYaw);
   angles.x() = osg::RadiansToDegrees(radians.x()); 
   radians.y() = atan2(sinPitch, cosPitch);
   angles.y() = osg::RadiansToDegrees(radians.y()); 

   radians.z() = atan2(sinRoll, cosRoll);
   angles.z() = osg::RadiansToDegrees(radians.z());
   return angles;
/*    
   // implementation converted from plib's sg.cxx
   // PLIB - A Suite of Portable Game Libraries
   // Copyright (C) 1998,2002  Steve Baker
   // For further information visit http://plib.sourceforge.net
   osg::Vec3 hpr;
   Matrix rotation = colMatrix; 
   osg::Matrix mat;

   osg::Vec3 col1(rotation(0, 0), rotation(0, 1), rotation(0, 2));
   double s = col1.length();

   const double magic_epsilon = 0.00001;
   if (s <= magic_epsilon)
   {
      hpr.set(0.0f, 0.0f, 0.0f);
      return hpr;
   }


   double oneOverS = 1.0f / s;
   for (int i = 0; i < 3; ++i)
   {
      for (int j = 0; j < 3; ++j)
      {
         mat(i, j) = rotation(i, j) * oneOverS;
      }
   }


   double sin_pitch = ClampUnity(mat(1, 2));
   double pitch = asin(sin_pitch);
   hpr[1] = osg::RadiansToDegrees(pitch);

   double cp = cos(pitch);

   if (cp > -magic_epsilon && cp < magic_epsilon)
   {
      double cr = ClampUnity(-mat(2,1));
      double sr = ClampUnity(mat(0,1));

      hpr[0] = 0.0f;
      hpr[2] = osg::RadiansToDegrees(atan2(sr,cr));
   }
   else
   {
      double one_over_cp = 1.0 / cp;
      double sr = ClampUnity(-mat(0,2) * one_over_cp);
      double cr = ClampUnity( mat(2,2) * one_over_cp);
      double sh = ClampUnity(-mat(1,0) * one_over_cp);
      double ch = ClampUnity( mat(1,1) * one_over_cp);

      if ((osg::equivalent(sh,0.0,magic_epsilon) && osg::equivalent(ch,0.0,magic_epsilon)) ||
          (osg::equivalent(sr,0.0,magic_epsilon) && osg::equivalent(cr,0.0,magic_epsilon)) )
      {
         cr = ClampUnity(-mat(2,1));
         sr = ClampUnity(mat(0,1));;

         hpr[0] = 0.0f;
      }
      else
      {
        hpr[0] = osg::RadiansToDegrees(atan2(sh, ch));
      }

      hpr[2] = osg::RadiansToDegrees(atan2(sr, cr));
   }
   return hpr;
*/
}

float ArtifactVis2::ClampUnity(float x)
{
   if (x >  1.0f) { return  1.0f; }
   if (x < -1.0f) { return -1.0f; }
   return x;
}
void ArtifactVis2::findAllModels()
{
string dir = ConfigManager::getEntry("Plugin.ArtifactVis2.3DModelFolder"); 

std::vector<DirFile*> entries0;

    string types = "kml";
getDirFiles(dir, entries0, types);
//cerr << "Entries " << entries0.size() << endl;

    string lastGroup = "";
    string lastGroup0 = "";
for(int i=0; i<entries0.size(); i++)
{

  if(entries0[i]->filetype == "folder")
  {
    if( entries0[i]->filename != "..")
    {
  //  cerr << "Files: " << entries0[i]->filename << endl;
    string dirSub = dir;
    dirSub.append(entries0[i]->filename);
    dirSub.append("/");
    std::vector<DirFile*> entriesSub;
    getDirFiles(dirSub, entriesSub, types);
    for(int n=0; n<entriesSub.size(); n++)
    {

             if(entriesSub[n]->filetype == "kml" && entries0[i]->filename != "models.kml" && entries0[i]->filename != "default_models.kml")
             {
		     string path = entriesSub[n]->path;
		     newSelectedFile = entriesSub[n]->path;
		     string kmlFilename = entriesSub[n]->filename;
		     path.append(kmlFilename);
		     string type = getKmlArray(path);
		     if(type == "Model")
		     {
			     int index = _models3d.size() - 1;
			     string group = _models3d[index]->group;
			   //  cerr << "Found SubFile: " << _models3d[index]->filename << endl;
				MenuCheckbox* site = new MenuCheckbox(_models3d[index]->name,false);
				site->setCallback(this);
				_showModelCB.push_back(site);
				 addToModelDisplayMenu(group, site);
		     }
		     else if(type == "PointCloud")
		     {
			     int index = _pointClouds.size() - 1;
			     string group = _pointClouds[index]->group;
			   //  cerr << "Found SubFile: " << _pointClouds[index]->filename << endl;
				MenuCheckbox* site = new MenuCheckbox(_pointClouds[index]->name,false);
				site->setCallback(this);
				_showPointCloudCB.push_back(site);
				 addToPcDisplayMenu(group, site);
		     }
             }
    }
    
    }
  }
  else
  { 
             if(entries0[i]->filetype == "kml" && entries0[i]->filename != "models.kml" && entries0[i]->filename != "default_models.kml")
             {
             string path  = entries0[i]->path;
             newSelectedFile = path;
             string kmlFilename = entries0[i]->filename;
            // cerr << "Found File: " << newSelectedName << endl;
             path.append(kmlFilename);
		     string type = getKmlArray(path);
		     if(type == "Model")
		     {
			     int index = _models3d.size() - 1;
			     string group = _models3d[index]->group;
			    // cerr << "Found SubFile: " << _models3d[index]->filename << endl;
				MenuCheckbox* site = new MenuCheckbox(_models3d[index]->name,false);
				site->setCallback(this);
				_showModelCB.push_back(site);
				 addToModelDisplayMenu(group, site);
		     }
		     else if(type == "PointCloud")
		     {
			     int index = _pointClouds.size() - 1;
			     string group = _pointClouds[index]->group;
			    // cerr << "Found SubFile: " << _pointClouds[index]->filename << endl;
				MenuCheckbox* site = new MenuCheckbox(_pointClouds[index]->name,false);
				site->setCallback(this);
				_showPointCloudCB.push_back(site);
				 addToPcDisplayMenu(group, site);
		     }
             newSelectedFile = "";
             newSelectedName = "";
		}

  }

}

}
string ArtifactVis2::getKmlArray(string file)
{
    FILE* fp = fopen(file.c_str(), "r");
 string completed = "";
 if (fp == NULL)
 {
        std::cerr << "Unable to open file: " << file << std::endl;
 }
 else
 {   
       // std::cerr << "Found file: " << file << std::endl;

    mxml_node_t* tree;
    tree = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
    fclose(fp);

   if (tree == NULL)
   {
        std::cerr << "Unable to parse XML file: " << file << std::endl;
        
   }
   else
   {
      //  std::cerr << "Parsing XML: " << file << std::endl;

    mxml_node_t* node = mxmlFindElement(tree, tree, "Placemark", NULL, NULL, MXML_DESCEND);

    if (true)
    {
        mxml_node_t* child = mxmlFindElement(node, tree, "type", NULL, NULL, MXML_DESCEND);
        string modelType = "";
        modelType = child->child->value.text.string;
        if(modelType == "Model" || modelType == "PointCloud")
        {
		child = mxmlFindElement(node, tree, "group", NULL, NULL, MXML_DESCEND);
		string group = child->child->value.text.string;
		child = mxmlFindElement(node, tree, "filetype", NULL, NULL, MXML_DESCEND);
		string filetype = child->child->value.text.string;

		child = mxmlFindElement(node, tree, "href", NULL, NULL, MXML_DESCEND);
		string filename = child->child->value.text.string;

		child = mxmlFindElement(node, tree, "name", NULL, NULL, MXML_DESCEND);
		string name = child->child->value.text.string;
		float trans[3];
		float scale[3];
		float rotDegrees[4];
		child = mxmlFindElement(node, tree, "altitudeMode", NULL, NULL, MXML_DESCEND);
		child = mxmlFindElement(node, tree, "longitude", NULL, NULL, MXML_DESCEND);
		trans[0] = atof(child->child->value.text.string);
		child = mxmlFindElement(node, tree, "latitude", NULL, NULL, MXML_DESCEND);
		trans[1] = atof(child->child->value.text.string);
		child = mxmlFindElement(node, tree, "altitude", NULL, NULL, MXML_DESCEND);
		trans[2] = atof(child->child->value.text.string);
		child = mxmlFindElement(node, tree, "x", NULL, NULL, MXML_DESCEND);
		scale[0] = atof(child->child->value.text.string);
		child = mxmlFindElement(node, tree, "y", NULL, NULL, MXML_DESCEND);
		scale[1] = atof(child->child->value.text.string);
		child = mxmlFindElement(node, tree, "z", NULL, NULL, MXML_DESCEND);
		scale[2] = atof(child->child->value.text.string);
		child = mxmlFindElement(node, tree, "heading", NULL, NULL, MXML_DESCEND);
		rotDegrees[0] = atof(child->child->value.text.string);
		child = mxmlFindElement(node, tree, "tilt", NULL, NULL, MXML_DESCEND);
		rotDegrees[1] = atof(child->child->value.text.string);
		child = mxmlFindElement(node, tree, "roll", NULL, NULL, MXML_DESCEND);
		rotDegrees[2] = atof(child->child->value.text.string);
		child = mxmlFindElement(node, tree, "w", NULL, NULL, MXML_DESCEND);
		bool degrees = true;
		if(child != NULL)
		{
		rotDegrees[3] = atof(child->child->value.text.string);
		degrees = false;
		}
		Vec3 pos;
		pos = Vec3(trans[0], trans[1], trans[2]);
		Quat rot;
		if(degrees)
		{

		rotDegrees[0] = DegreesToRadians(rotDegrees[0]);
		rotDegrees[1] = DegreesToRadians(rotDegrees[1]);
		rotDegrees[2] = DegreesToRadians(rotDegrees[2]);
		rot = osg::Quat(rotDegrees[0], osg::Vec3d(1,0,0),rotDegrees[1], osg::Vec3d(0,1,0),rotDegrees[2], osg::Vec3d(0,0,1)); 
		}
		else
		{
		 // cerr << "As Quats\n";
		  rot = Quat(rotDegrees[0],rotDegrees[1],rotDegrees[2],rotDegrees[3]);
		}
	    Model* newModel = new Model();
            newModel->loaded = false;
	    newModel->name = name;
	    newModel->filename = filename;
            string fullpath = getPathFromFilePath(file);
            fullpath.append(filename);
	    newModel->fullpath = fullpath;
	    newModel->group = group;
	    newModel->filetype = filetype;
	    newModel->modelType = modelType;
	    newModel->pos = pos; 
	    newModel->rot = rot;
	    newModel->origPos = pos; 
	    newModel->origRot = rot;
	    newModel->scale = scale[0]; 
	    newModel->origScale = scale[0];
            if(modelType == "Model")
            {
	    _models3d.push_back(newModel);
            }
            else if(modelType == "PointCloud")
            {
	    _pointClouds.push_back(newModel);
            }
	    completed = modelType; 
       }
    }
   }
  }
return completed;
}
void ArtifactVis2::addToModelDisplayMenu(string group, cvr::MenuCheckbox* site)
{
                   bool groupExists = false;
                   int m = 0; 
	    	   for (int z = 0; z < _modelGroup.size(); z++)
	    	   {
                      if(_modelGroup[z] == group)
                      {
			groupExists = true;
 			m = z;
			break;
                      }
	 	   }
                   if(groupExists)
                   {
		      _modelMenus[m]->addItem(site);


                   }
                   else
                   {
                 //  cerr << "group: " << group << endl;
                     SubMenu*  newMenu = new SubMenu(group, group);
                      newMenu->setCallback(this);
                      newMenu->addItem(site);
		      _modelMenus.push_back(newMenu);
		      _modelGroup.push_back(group);
	 	   
		   }

}
void ArtifactVis2::addToPcDisplayMenu(string group, cvr::MenuCheckbox* site)
{
                   bool groupExists = false;
                   int m = 0; 
	    	   for (int z = 0; z < _pcGroup.size(); z++)
	    	   {
                      if(_pcGroup[z] == group)
                      {
			groupExists = true;
 			m = z;
			break;
                      }
	 	   }
                   if(groupExists)
                   {
		      _pcMenus[m]->addItem(site);


                   }
                   else
                   {
                  // cerr << "group: " << group << endl;
                     SubMenu*  newMenu = new SubMenu(group, group);
                      newMenu->setCallback(this);
                      newMenu->addItem(site);
		      _pcMenus.push_back(newMenu);
		      _pcGroup.push_back(group);
	 	   
		   }

}
std::string ArtifactVis2::getPathFromFilePath(string filepath)
{

                //Get Full path
 size_t found=filepath.find_last_of("/");
    string path;
            if (found!=string::npos)
	    {
                 int start = int(found);
                 path = filepath;
                 path.erase(start,(path.length()-start));
                 path.append("/"); 
                  // cerr << "path: " << path << endl;
            }
return path;
}
std::string ArtifactVis2::getFileFromFilePath(string filepath)
{

                //Get Full path
 size_t found=filepath.find_last_of("/");
    string path;
            if (found!=string::npos)
	    {
                 int start = int(found);
                 path = filepath;
                 path.erase(0,(start+1)); 
                   cerr << "filename: " << path << endl;
            }
return path;
}
void ArtifactVis2::turnOffAll()
{

    _bookmarkPanel->setVisible(false);
    _bookmarksMenu->setValue(false);
    _utilsPanel->setVisible(false);
    _utilsMenu->setValue(false);
    _filePanel->setVisible(false);
    _fileMenu->setValue(false);
    _qsPanel->setVisible(false);
    _qsMenu->setValue(false);
    for (int i = 0; i < _annotations.size(); i++)
    {
        if(_annotations[i]->visible)
        {
        _annotations[i]->active = false;
        _annotations[i]->visible = false;
	_annotations[i]->visibleMap->setValue(false);
        _annotations[i]->so->setMovable(false);
        _annotations[i]->activeMap->setValue(false);
	_annotations[i]->so->detachFromScene();
        _annotations[i]->connectorNode->setNodeMask(0);
        }
    }
    for (int i = 0; i < _lineGroups.size(); i++)
    {
        if(_lineGroups[i]->visible)
        {
        _lineGroups[i]->active = false;
        _lineGroups[i]->visible = false;
	_lineGroups[i]->visibleMap->setValue(false);
        _lineGroups[i]->so->setMovable(false);
        _lineGroups[i]->activeMap->setValue(false);
	_lineGroups[i]->so->detachFromScene();
        }
    }
    for (int i = 0; i < _artifactAnnoTrack.size(); i++)
    {
       int q = _artifactAnnoTrack[i]->q;
       int art = _artifactAnnoTrack[i]->art;
       if (!_query[q]->artifacts[art]->annotation->visibleMap->getValue())
       {
	 _query[q]->artifacts[art]->model->pVisibleMap->setValue(false);
	 _query[q]->artifacts[art]->annotation->active = false;
	 _query[q]->artifacts[art]->annotation->so->setMovable(false);
	 _query[q]->artifacts[art]->annotation->activeMap->setValue(false);
	 _artifactAnnoTrack[i]->active = false;
	 _query[q]->artifacts[art]->annotation->so->detachFromScene();
	 _query[q]->artifacts[art]->annotation->connectorNode->setNodeMask(0);
	 _query[q]->artifacts[art]->annotation->visible = false;
       }
    }
    for (int i = 0; i < _artifactModelTrack.size(); i++)
    {
       int q = _artifactModelTrack[i]->q;
       int art = _artifactModelTrack[i]->art;
       if (_query[q]->artifacts[art]->model->visible)
       {
	 _query[q]->artifacts[art]->model->active = false;
	 _query[q]->artifacts[art]->model->so->setMovable(false);
	 _query[q]->artifacts[art]->model->activeMap->setValue(false);
	 _artifactModelTrack[i]->active = false;
	 _query[q]->artifacts[art]->model->so->detachFromScene();
	 _query[q]->artifacts[art]->model->visible = false;
       }
    }
    for (int i = 0; i < _queryOption.size(); i++)
    {
      int n = _querySfIndex[i];
      if (_queryOption[i]->getValue())
      {
	_query[n]->sphereRoot->setNodeMask(0);
        _queryOption[i]->setValue(false);
      }
    }
    for (int i = 0; i < _queryOptionLoci.size(); i++)
    {
      int n = _queryLociIndex[i];
      if (_queryOptionLoci[i]->getValue())
      {
	_query[n]->sphereRoot->setNodeMask(0);
        _queryOptionLoci[i]->setValue(false);
      }
    }
    for (int i = 0; i < _showModelCB.size(); i++)
    {
        if (_showModelCB[i]->getValue())
        {
        _showModelCB[i]->setValue(false);
	_models3d[i]->so->detachFromScene();
	_models3d[i]->visible = false;
	_models3d[i]->active = false;
	_models3d[i]->visibleMap->setValue(false);
	_models3d[i]->activeMap->setValue(false);
       }
    }
    for (int i = 0; i < _showPointCloudCB.size(); i++)
    {
        if (_showPointCloudCB[i]->getValue())
        {
        _showPointCloudCB[i]->setValue(false);
	_pointClouds[i]->so->detachFromScene();
	_pointClouds[i]->visible = false;
	_pointClouds[i]->active = false;
	_pointClouds[i]->visibleMap->setValue(false);
	_pointClouds[i]->activeMap->setValue(false);
       }
    }
    //Resets View
    if(true)
    {
	osg::Matrix m;
	SceneManager::instance()->setObjectMatrix(m);
	SceneManager::instance()->setObjectScale(1.0);
    }

}
void ArtifactVis2::saveBookmark(osg::Matrix headMat, float scale)
{
    int newFly = _flyplace->name.size();

    stringstream ss;
    ss << newFly;
    string flyname = ss.str();

        _flyplace->name.push_back(flyname);
        _flyplace->scale.push_back(scale);
        _flyplace->x.push_back(headMat.getTrans().x());
        _flyplace->y.push_back(headMat.getTrans().y());
        _flyplace->z.push_back(headMat.getTrans().z());
        _flyplace->rx.push_back(headMat.getRotate().x());
        _flyplace->ry.push_back(headMat.getRotate().y());
        _flyplace->rz.push_back(headMat.getRotate().z());
        _flyplace->rw.push_back(headMat.getRotate().w());

        MenuButton* gotoP = new MenuButton(flyname);  
        gotoP->setCallback(this);
        _bookmarkPanel->addMenuItem(gotoP);
        _goto.push_back(gotoP);

//Create Xml

    mxml_node_t *xml;    /* <?xml ... ?> */
    mxml_node_t *flyto;
    mxml_node_t *placemark;
    mxml_node_t *_name;   /* <name> */
    mxml_node_t *_scale;
    mxml_node_t *_x;
    mxml_node_t *_y;
    mxml_node_t *_z;
    mxml_node_t *_rx;
    mxml_node_t *_ry;
    mxml_node_t *_rz;
    mxml_node_t *_rw;

xml = mxmlNewXML("1.0");
            flyto = mxmlNewElement(xml, "flyto");

    for (int i = 0; i < (newFly+1); i++)
    {
	stringstream buffer;
	buffer << _flyplace->scale[i];
        float c_scale = _flyplace->scale[i];
	   string x_scale = buffer.str();
	   buffer.str("");
	   buffer << (_flyplace->x[i]/c_scale);
	   string x = buffer.str();
	   buffer.str("");
	   buffer << (_flyplace->y[i]/c_scale);
	   string y = buffer.str();
	   buffer.str("");
	   buffer << (_flyplace->z[i]/c_scale);
	   string z = buffer.str();
	   buffer.str("");
	   buffer << (_flyplace->rx[i]);
	   string rx = buffer.str();
	   buffer.str("");
	   buffer << (_flyplace->ry[i]);
	   string ry = buffer.str();
	   buffer.str("");
	   buffer << (_flyplace->rz[i]);
	   string rz = buffer.str();
	   buffer.str("");
	   buffer << (_flyplace->rw[i]);
	   string rw = buffer.str();
	   buffer.str("");
         
           string name = _flyplace->name[i]; 
           placemark = mxmlNewElement(flyto, "Placemark");
              _name = mxmlNewElement(placemark, "name");
              mxmlNewText(_name, 0, name.c_str());
              _scale = mxmlNewElement(placemark, "scale");
              mxmlNewText(_scale, 0, x_scale.c_str());
              _x = mxmlNewElement(placemark, "x");
              mxmlNewText(_x, 0, x.c_str());
              _y = mxmlNewElement(placemark, "y");
              mxmlNewText(_y, 0, y.c_str());
              _z = mxmlNewElement(placemark, "z");
              mxmlNewText(_z, 0, z.c_str());
              _rx = mxmlNewElement(placemark, "rx");
              mxmlNewText(_rx, 0, rx.c_str());
              _ry = mxmlNewElement(placemark, "ry");
              mxmlNewText(_ry, 0, ry.c_str());
              _rz = mxmlNewElement(placemark, "rz");
              mxmlNewText(_rz, 0, rz.c_str());
              _rw = mxmlNewElement(placemark, "rw");
              mxmlNewText(_rw, 0, rw.c_str());
    } 
//.......................................................
//Save File

  const char *ptr;
    ptr = "";
  ptr = mxmlSaveAllocString(xml, MXML_NO_CALLBACK);
    //cout << ptr;
    FILE *fp;
    
    string filename = ConfigManager::getEntry("Plugin.ArtifactVis2.Database").append("flyto.xml");
    
    fp = fopen(filename.c_str(), "w");

    fprintf(fp, ptr);

    fclose(fp);
 
}
