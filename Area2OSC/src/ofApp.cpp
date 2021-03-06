#include "ofApp.h"
#define APP_NAME "Augmenta area2OSC"

using namespace ofxSpout2;

//_______________________________________________________________
//_____________________________SETUP_____________________________
//_______________________________________________________________

//--------------------------------------------------------------
void ofApp::setup(){

	// If we are building a debug binary, all the outputs will be shown
#ifdef DEBUG
	ofSetLogLevel(OF_LOG_VERBOSE);
	ofSetWindowTitle(APP_NAME"Debug");
#else
	ofSetLogLevel(OF_LOG_NOTICE);
	ofSetWindowTitle(APP_NAME);
#endif

	// Limit framerate to 60fps
	ofSetFrameRate(60);

	// Init function is used to set default variables that can be changed.
	// For example, GUI variables or preferences.xml variables.
	init();
	loadPreferences();


	// Important : call those function AFTER init,
	// because init() will define all default values
	m_iNextFreeId = 0;
	setupGUI();
	setupOSC();

	m_oPeople = AugmentaReceiver.getPeople();
	m_oActualScene = AugmentaReceiver.getScene();

	if (m_oActualScene->width == 0 || m_oActualScene->height == 0){
		m_iFboWidth = 1024;
		m_iFboHeight = 768;
	}else{
		m_iFboWidth = m_oActualScene->width;
		m_iFboHeight = m_oActualScene->height;
	}

    /*
     Visuals will be drawn in a FBO for several reasons :
     - We need a texture reference to send our visuals with Syphon or Spout outside the app
     - You can draw visuals larger than your app or screen resolution
     - FBOs are easy to draw wherever you want in screen space to build your UI
     
     */
    
    // Allocating a FBO of the size defined in preferences.xml (default : window size), can be any size
	m_fbo.allocate(m_iFboWidth,m_iFboHeight,GL_RGBA);

    #if MAC_OS_X_VERSION_10_6
    // Setup Syphon output
    m_syphonServer.setName(APP_NAME);
    #endif
}

//--------------------------------------------------------------
void ofApp::init(){
   
	//--------------------------------------------
	//In case of reset
	if (m_vAreaPolygonsVector.size() > 0){
		if (!m_vAreaPolygonsVector[m_vAreaPolygonsVector.size() - 1].isCompleted()){
			m_vAreaPolygonsVector.pop_back();
		}
	}
	for (int i = 0; i < m_vAreaPolygonsVector.size(); ++i){
		m_vAreaPolygonsVector[i].hasBeenSelected(false);
	}

    //--------------------------------------------
    // Change default values here.
    //--------------------------------------------
    // App default values (preferences.xml)
    m_bHideInterface = false;
    m_bLogToFile = false;
    m_iFboWidth = 1024;
    m_iFboHeight = 768;
	m_sOutputIp = "127.0.0.1";
	m_sOutputPort = 13000;
	m_sInputPort = 12000;
	m_sPolyName = std::to_string(0);
    //m_sInputPort = 13000;
    //m_sOutputPort = 12000;
    //m_sOutputIp = "127.0.0.1";
    m_sReceiverOscDisplay = "Listening to OSC on port m_sInputPort\n";

	m_sAugmentaOscDiplay = "Listening to Augmenta OSC on port  m_sInputPort\n";
	m_iIndicePolygonSelected = -1;
    m_fPointRadius = 20;
	m_iLinesWidthSlider = 2;
	m_bRedondanteMode = false;
	m_oToggleClearAll = false;
	m_oToggleDeleteLastPoly = false;
	m_bEditMode = false;
	m_bSelectMode = false;
	m_iNumberOfAreaPolygons = m_vAreaPolygonsVector.size();
	m_iRadiusClosePolyZone = 15;
	m_oOldMousePosition = ofVec2f(0,0);
	m_iAntiBounce = 100;
	m_fZoomCoef = 1.0;
	m_bSendFbo = false;
	m_sScreenResolution = ofToString(m_iWidthRender) +" x "+ ofToString(m_iHeightRender);
	m_sSendFboResolution = ofToString(m_iFboWidth) + " x " + ofToString(m_iFboHeight);


	
}

//--------------------------------------------------------------
void ofApp::setupGUI(){

	// Add listeners before setting up so the initial values are correct
	// Listeners can be used to call a function when a UI element has changed.
	// Don't forget to call ofRemoveListener before deleting any instance that is listening to an event, to prevent crashes. Here, we will call it in exit() method.
	m_bResetSettings.addListener(this, &ofApp::reset);

	// Setup GUI panel
	m_gui.setup();
	m_gui.setName("GUI Parameters");

	// Add content to GUI panel
	m_gui.add(m_sInputPort.setup("Input Port ", m_sInputPort));
	m_gui.add(m_sOutputIp.setup("Output IP ", m_sOutputIp));
	m_gui.add(m_sOutputPort.setup("Output Port ", m_sOutputPort));
	m_gui.add(m_sScreenResolution.setup("Window res ", m_sScreenResolution));
	m_gui.add(m_sSendFboResolution.setup("Fbo res ", m_sSendFboResolution));
	m_gui.add(m_sFramerate.setup("FPS", m_sFramerate));
	m_gui.add(m_sNumberOfAreaPolygons.setup("Number of polygons", m_sNumberOfAreaPolygons));
	m_gui.add(m_sPolyName.setup("Polygone name ", std::to_string(0)));
	m_gui.add((m_fZoomCoef.setup("Zoom", 1, 0.3, 2)));
	m_gui.add(m_bResetSettings.setup("Reset Settings", m_bResetSettings));

	// guiFirstGroup parameters ---------------------------
	string sFirstGroupName = "Polygons";
	m_guiFirstGroup.setName(sFirstGroupName);     // Name the group of parameters (important if you want to apply color to your GUI)
	//m_guiFirstGroup.add(m_sPolyName.setup("Polygone name ", m_sPolyName));
	m_guiFirstGroup.add((m_oToggleClearAll.setup("Delete all polygons", m_oToggleClearAll))->getParameter());
	m_guiFirstGroup.add((m_oToggleDeleteLastPoly.setup("Delete last polygon", m_oToggleDeleteLastPoly))->getParameter());
	m_gui.add(m_guiFirstGroup);     // When all parameters of the group are set up, add the group to the gui panel.

	// guiSecondGroup parameters ---------------------------
	string sSecondGroupName = "OSC";
	m_guiSecondGroup.setName(sSecondGroupName);
	m_guiSecondGroup.add((m_bRedondanteMode.setup("Send all event", m_bRedondanteMode))->getParameter());
	m_guiSecondGroup.add(m_iAntiBounce.setup("Anti bounce ms",100,1,400)->getParameter());
	m_gui.add(m_guiSecondGroup);

	#ifdef WIN32
	string sThridGroupName = "SPOUT";
	#elif __APPLE__
	string sThridGroupName = "SYPHON";
	#else
	string sThridGroupName = "FBO";
	#endif
	

	m_guiThirdGroup.setName(sThridGroupName);
	m_guiThirdGroup.add((m_bSendFbo.setup("on/off", m_bSendFbo))->getParameter());
	
	m_gui.add(m_guiThirdGroup);

	// You can add colors to your GUI groups to identify them easily
	// Example of beautiful colors you can use : salmon, orange, darkSeaGreen, teal, cornflowerBlue...
	m_gui.getGroup(sFirstGroupName).setHeaderBackgroundColor(ofColor::salmon);    // Parameter group must be get with its name defined in setupGUI()
	m_gui.getGroup(sFirstGroupName).setBorderColor(ofColor::salmon);
	m_gui.getGroup(sSecondGroupName).setHeaderBackgroundColor(ofColor::orange);
	m_gui.getGroup(sSecondGroupName).setBorderColor(ofColor::orange);
	m_gui.getGroup(sThridGroupName).setHeaderBackgroundColor(ofColor::cornflowerBlue);
	m_gui.getGroup(sThridGroupName).setBorderColor(ofColor::cornflowerBlue);

	// Load autosave settings
	if (ofFile::doesFileExist("autosave.xml")){
		m_gui.loadFromFile("autosave.xml");
	}
}

//--------------------------------------------------------------
void ofApp::setupOSC(){

	//m_sReceiverOscDisplay = "Listening to OSC on port " + std::to_string(m_sInputPort);
	//m_oscReceiver.stop();
	//AugmentaReceiver.stop();
	//m_oscSender.clear();
	
	AugmentaReceiver.connect(m_sInputPort);
	m_oscReceiver.setup(m_sInputPort);
	m_oscSender.setup(m_sOutputIp, m_sOutputPort);

	ofLogVerbose("OSC settings updated");
}

//--------------------------------------------------------------
void ofApp::reset(){
    init();
}

//_______________________________________________________________
//_____________________________UPDATE____________________________
//_______________________________________________________________

//--------------------------------------------------------------
void ofApp::update(){

	m_sScreenResolution = ofToString(ofGetWindowWidth()) + " x " + ofToString(ofGetWindowHeight());
	m_sSendFboResolution = ofToString(m_iFboWidth) + " x " + ofToString(m_iFboHeight);

	if (m_oToggleDeleteLastPoly){
		deleteLastPolygon();
		m_oToggleDeleteLastPoly = false;
	}
	if (m_oToggleClearAll){
		deleteAllPolygon();
		m_oToggleClearAll = false;
	}

	//Update Augmenta
	m_oPeople = AugmentaReceiver.getPeople();
	m_oActualScene = AugmentaReceiver.getScene();

	//Update GUI
	m_iNumberOfAreaPolygons = m_vAreaPolygonsVector.size();

	////Update osc settings
	//if (m_oscReceiver.getPort() != m_sInputPort || m_oscSender.getPort() != m_sOutputPort || m_oscSender.getHost().compare(m_sOutputIp) != 0) {
	//	setupOSC();
	//}
	/*if (m_oscReceiver.getPort() != m_sInputPort) {
		ofLogVerbose("tachatte");
		try {
			m_oscReceiver.setup(m_sInputPort);
			AugmentaReceiver.connect(m_sInputPort);
		}
		catch (std::exception&e) {
			ofLogWarning("setupOSC") << "Error : " << ofToString(e.what());
			m_sReceiverOscDisplay = "\n/!\\ ERROR : Could not bind to OSC port m_sInputPort\n";
		}
	}
	if (m_oscSender.getPort() != m_sOutputPort || m_oscSender.getHost().compare(m_sOutputIp) == 0) {
		m_oscSender.setup(m_sOutputIp, m_sOutputPort);
	}*/
		
	//update name
	if (m_bSelectMode) {
		m_vAreaPolygonsVector[m_iIndicePolygonSelected].setPolyName(m_sPolyName);
	}
	if (m_bEditMode) {
		m_vAreaPolygonsVector[m_iNumberOfAreaPolygons - 1].setPolyName(m_sPolyName);
	}

	//Update Colision
	for (size_t i = 0; i < m_iNumberOfAreaPolygons; i++){
		if (m_vAreaPolygonsVector[i].isCompleted()){
			m_vAreaPolygonsVector[i].update(m_oPeople, m_iAntiBounce);
		}
	}
	
	//Osc
	sendOSC();
}

//--------------------------------------------------------------
void ofApp::fboSizeHaveChanged(int a_iNewWidth, int a_iNewHeight){
	if (a_iNewWidth != 0 && a_iNewHeight != 0){
		if (m_iFboWidth != a_iNewWidth || m_iFboHeight != a_iNewHeight){
			m_iFboWidth = a_iNewWidth;
			m_iFboHeight = a_iNewHeight;
			m_fbo.allocate(m_iFboWidth, m_iFboHeight, GL_RGBA);
		}
	}
}

//--------------------------------------------------------------
bool ofApp::isInsideAPolygon(ofVec2f a_oPoint){
	//We don't count the last poly because it his in construction
	if (m_bEditMode){
		for (int i = 0; i < m_vAreaPolygonsVector.size() - 1; i++){
			if (m_vAreaPolygonsVector[i].isPointInPolygon(ofPoint(static_cast<float>(a_oPoint.x) / m_iFboWidth, static_cast<float>(a_oPoint.y) / m_iFboHeight))){
				return true;
			}
		}
		return false;
	}
	else{
		for (int i = 0; i < m_vAreaPolygonsVector.size(); i++){
			if (m_vAreaPolygonsVector[i].isPointInPolygon(ofPoint(static_cast<float>(a_oPoint.x) / m_iFboWidth, static_cast<float>(a_oPoint.y) / m_iFboHeight))){
				return true;
			}
		}
		return false;
	}
}

//_______________________________________________________________
//_____________________________DRAW______________________________
//_______________________________________________________________

//--------------------------------------------------------------
void ofApp::drawAreaPolygons(){
	for (int i = 0; i < m_vAreaPolygonsVector.size(); ++i){
		m_vAreaPolygonsVector[i].draw(m_iFboWidth , m_iFboHeight );
	}
}

//--------------------------------------------------------------
void ofApp::drawAugmentaPeople(){
	ofPoint centroid;
	ofPushStyle();
	ofSetColor(ofColor(93,224,133,200));
	for (int i = 0; i<m_oPeople.size(); i++){
		centroid = m_oPeople[i]->centroid;
		ofCircle(centroid.x * m_iFboWidth , centroid.y * m_iFboHeight , 10);
	}
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::draw(){	
	
	fboSizeHaveChanged(m_oActualScene->width, m_oActualScene->height);

	m_fbo.begin();
	ofClear(ofColor(52,53,46)); // Clear FBO content to black
	ofEnableDepthTest();
	
	drawAugmentaPeople();
	drawAreaPolygons();
	
	ofDisableDepthTest();
	m_fbo.end();

	if (m_bSendFbo){
		sendVisuals();
	}

    // Draw interface
    if(m_bHideInterface){
        drawHiddenInterface();
    } else{
        drawInterface();
		
    }
}

//--------------------------------------------------------------
// Send the FBO to Syphon (OSX) or Spout (WIN32)
void ofApp::sendVisuals(){
    
    #ifdef WIN32
    // On Windows, use Spout
	m_spoutSender.sendTexture(m_fbo.getTextureReference(), APP_NAME);
    #elif MAC_OS_X_VERSION_10_6
    // On Mac OSX, use Syphon
    m_syphonServer.publishTexture(&m_fbo.getTextureReference());
    #endif
}

//--------------------------------------------------------------
// Draw the interface of your app : visuals, GUI, debug content...
void ofApp::drawInterface(){
    
	int min = 700;
	int max = 1000;

    ofBackground(ofColor::darkGray); // Clear screen

	m_iWidthRender = m_iFboWidth;
	m_iHeightRender = m_iFboHeight;


	float fFboRatio = (float)m_iFboWidth / (float)m_iFboHeight;

	// Portrait or square mode Height >= Width
	if (fFboRatio <= 1){
		if (m_iHeightRender > max){
			m_iHeightRender = max;
			m_iWidthRender = m_iHeightRender * fFboRatio;
		}
		if (m_iWidthRender < min){
			m_iWidthRender = min;
			m_iHeightRender = m_iWidthRender / fFboRatio;
		}
	}
	// Landscape mode Height < Width
	if (fFboRatio > 1){
		if (m_iWidthRender > max){
			m_iWidthRender = max;
			m_iHeightRender = m_iWidthRender / fFboRatio;
		}
		if (m_iHeightRender < min){
			m_iHeightRender = min;
			m_iWidthRender = m_iHeightRender * fFboRatio;
		}
	}
	
	m_fbo.draw(0, 0, m_iWidthRender * m_fZoomCoef, m_iHeightRender * m_fZoomCoef);
	ofSetWindowShape(m_iWidthRender * m_fZoomCoef, m_iHeightRender * m_fZoomCoef);
	
	// Update the UI 
    m_sFramerate = ofToString(ofGetFrameRate());
	m_sNumberOfAreaPolygons = ofToString(m_iNumberOfAreaPolygons);
	m_gui.draw();

}

//--------------------------------------------------------------
// Minimalist interface with a black background and some information
void ofApp::drawHiddenInterface(){
    
    ofBackground(ofColor::black);
    
    ofPushStyle();
    ofSetColor(ofColor::white);

	ofDrawBitmapString("FPS: " +
		ofToString(ofGetFrameRate()) + "\n" +
		"Sending OSC to " + "m_sOutputIp" + ":" + "m_sOutputPort" + "\n" + m_sAugmentaOscDiplay
		+ "\n" +
		"Fbo width : " + ofToString(AugmentaReceiver.getScene()->width) + "\n" +
		"Fbo height : " + ofToString(AugmentaReceiver.getScene()->height) + "\n\n-" +
		"---------------------------------------\n"
		"\n[h] to unhide interface\n" \
		"[ctrl+s] / [cmd+s] to save settings and the currents polygons\n" \
		"[ctrl+l] / [cmd+l] to load last saved settings and polygons\n" \
		"[ctrl+z] to delete the last polygon created or the current polygon\n" \
		"[r] / [R] to delete all the polygons you have created\n" \
		"[del] / [backSpace] to delete the selected polygon\n" \
		"[-] To zoom in(only for a better appreciation on the screen).\n"\
		"[+] To zoom out(only for a better appreciation on the screen).\n"\
		"[left click] to create a new point or polygon\n" \
		"[right click] to delete the last point created\n" \
		"[left click] inside a polygon to select it /outside a polygon to deselect it\n\n"

		"---------------------------------------\n" \
		"\nTo optimize performance : \n\n" \
		"  - Stay in this hidden interface mode\n" \
		"  - Minimize this window\n" \
		"\nNote : Your settings are saved when the app quits and are loaded at startup. (autosave feature)\n" \
		"The dimensions will be the same as the ones sent by Augmenta.\n" \
		"You can change the osc messages of a polygon in the preferences.xml file."\

	

                       ,20,20);
    
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::deleteLastPolygon() {
    
    if (m_vAreaPolygonsVector.size() >= 1){
        if (m_bSelectMode){
            m_vAreaPolygonsVector[m_iIndicePolygonSelected].hasBeenSelected(false);
            m_iIndicePolygonSelected = -1;
            m_bSelectMode = false;
        }
        m_bEditMode = false;
        m_vAreaPolygonsVector.pop_back();
        ofLogVerbose("deleteLastPolygon", "Last AreaPolygon is now deleted ");
    }
}

//--------------------------------------------------------------
void ofApp::deleteAllPolygon(){
	if (m_bSelectMode){
		m_vAreaPolygonsVector[m_iIndicePolygonSelected].hasBeenSelected(false);
		m_iIndicePolygonSelected = -1;
		m_bSelectMode = false;
	}
	m_vAreaPolygonsVector.clear();
	m_bEditMode = false;
	m_iNextFreeId = 0;
	ofLogVerbose("deleteAllPolygon", "All AreaPolygons are now deleted ");
}

//_______________________________________________________________
//_____________________________INPUT_____________________________
//_______________________________________________________________

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
 
    switch(key){
        // Modifier keys
        case OF_KEY_SHIFT:
            m_iModifierKey = OF_KEY_SHIFT;
            break;
        case OF_KEY_ALT:
            m_iModifierKey = OF_KEY_ALT;
            break;
        case OF_KEY_CONTROL:
            m_iModifierKey = OF_KEY_CONTROL;
            break;
        case OF_KEY_COMMAND:
            m_iModifierKey = OF_KEY_COMMAND;
            break;

		case '+':
			if (m_fZoomCoef + 0.2 < m_fZoomCoef.getMax()){
				m_fZoomCoef = m_fZoomCoef + 0.2;
			}
			else{
				m_fZoomCoef = m_fZoomCoef.getMax();
			}
			break;

		case '-':
			if (m_fZoomCoef - 0.2 > m_fZoomCoef.getMin()){
				m_fZoomCoef = m_fZoomCoef-0.2;
			}
			else{
				m_fZoomCoef = m_fZoomCoef.getMin();
			}
			break;

        case 'f':
        case 'F':
            ofToggleFullscreen();
            break;
            
        case 'h':
        case 'H':
            // Hide UI to save some performances (or for discretion)
            m_bHideInterface = !m_bHideInterface;
            break;
            
        case 's':
        case 'S':
            // CTRL+S or CMD+S
            if(m_iModifierKey == OF_KEY_CONTROL || m_iModifierKey == OF_KEY_COMMAND){
                // Save settings
                saveSettings();
				ofLogVerbose("keyPressed", "Settings have been successfully saved ");
            }
            break;
        
        case 'z':
        case 'Z':
            // CTRL+Z or CMD+Z
            if(m_iModifierKey == OF_KEY_CONTROL || m_iModifierKey == OF_KEY_COMMAND){
                deleteLastPolygon();
            }
            break;
            
        case 'l':
        case 'L':
            // CTRL+L or CMD+L
            if(m_iModifierKey == OF_KEY_CONTROL || m_iModifierKey == OF_KEY_COMMAND){
                // Load settings
                loadSettings();
				ofLogVerbose("keyPressed", "Settings have been successfully loaded ");
            }
            break;
            
        case 19:
            // CTRL+S or CMD+S
            saveSettings();
			ofLogVerbose("keyPressed", "Settings have been successfully saved ");
            break;
            
        case 12:
            // CTRL+L or CMD+L
            loadSettings();
			ofLogVerbose("keyPressed", "Settings have been successfully loaded ");
            break;

			//CTRL + Z
			//Delete Last AreaPolygon
		case 26:
            deleteLastPolygon();
			break;

		//Delete the selected poly
		case OF_KEY_DEL :			
		case OF_KEY_BACKSPACE :
			if (m_vAreaPolygonsVector.size() >= 1){
				if (m_bSelectMode && !m_bEditMode){
					m_vAreaPolygonsVector.erase(m_vAreaPolygonsVector.begin() + m_iIndicePolygonSelected);
					m_iIndicePolygonSelected = -1;
					m_bSelectMode = false;		
					ofLogVerbose("keyPressed", "The selected polygon is now deleted");
				}
			}
			break;
		
		//Delete all AreaPolygons
		case 'r':
		case 'R':			
			deleteAllPolygon();		
			break;

        //Move the selected polygon
		if (m_bSelectMode){
			case OF_KEY_LEFT:
				if (m_iIndicePolygonSelected != -1){
					m_vAreaPolygonsVector[m_iIndicePolygonSelected].moveLeft();
					m_vAreaPolygonsVector[m_iIndicePolygonSelected].setPolygonCentroid();
					ofLogVerbose("keyPressed", "go left !");
				}
				break;

			case OF_KEY_RIGHT:
				if (m_iIndicePolygonSelected != -1){
					m_vAreaPolygonsVector[m_iIndicePolygonSelected].moveRight();
					m_vAreaPolygonsVector[m_iIndicePolygonSelected].setPolygonCentroid();
					ofLogVerbose("keyPressed", "go right !");
				}
				break;

			case OF_KEY_UP:
				if (m_iIndicePolygonSelected != -1){
					m_vAreaPolygonsVector[m_iIndicePolygonSelected].moveUp();
					m_vAreaPolygonsVector[m_iIndicePolygonSelected].setPolygonCentroid();
					ofLogVerbose("keyPressed", "go up !");
				}
				break;

			case OF_KEY_DOWN:			
				if (m_iIndicePolygonSelected != -1){
					m_vAreaPolygonsVector[m_iIndicePolygonSelected].moveDown();
					m_vAreaPolygonsVector[m_iIndicePolygonSelected].setPolygonCentroid();
					ofLogVerbose("keyPressed", "go down !");
				}
				break;
		}
        default:
            break;
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
    switch (key){
            
        // Release modifier key
        case OF_KEY_SHIFT:
        case OF_KEY_ALT:
        case OF_KEY_CONTROL:
        case OF_KEY_COMMAND:
            m_iModifierKey = -1;
            break;
           
        default:
            break;
    }
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	ofPoint temp = transformMouseCoord(x, y);
	x = temp.x;
	y = temp.y;

	ofVec2f movement = ofVec2f(m_oOldMousePosition.x - x, m_oOldMousePosition.y - y);

	if (button == 0){
		if (!m_bEditMode){
			if (m_bSelectMode){
				m_vAreaPolygonsVector[m_iIndicePolygonSelected].move(static_cast<float>(movement.x) / m_iFboWidth, static_cast<float>(movement.y) / m_iFboHeight);
				m_vAreaPolygonsVector[m_iIndicePolygonSelected].setPolygonCentroid();
			}
		}
		else {
			m_vAreaPolygonsVector[m_vAreaPolygonsVector.size() - 1].moveLastPoint(static_cast<float>(x) / m_iFboWidth, static_cast<float>(y) / m_iFboHeight);
		}
	}
	m_oOldMousePosition = ofVec2f(x, y);
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

	ofPoint temp = transformMouseCoord(x, y);
	x = temp.x;
	y = temp.y;
	m_oOldMousePosition = ofVec2f(x, y);
	bool isLastPoint = false;

	if (button == 2 && !m_bSelectMode){
		//One AreaPolygon is not finish
		if (m_bEditMode){
			if (m_vAreaPolygonsVector[m_vAreaPolygonsVector.size() - 1].removeLastPoint()){
				//if the value return by the remove last point value 
			}
			else{
				m_vAreaPolygonsVector.pop_back();
				m_bEditMode = false;
			}
		}
	}
	
	if (button == 0){
		//Creation of poly
		if (!m_bSelectMode && !isInsideAPolygon(ofVec2f(x, y))){
			//One AreaPolygon is not finish
			if (m_bEditMode){
				//Is closing the poly

				ofVec2f temp = m_vAreaPolygonsVector[m_iNumberOfAreaPolygons - 1].getPoint(0);
				temp.x = temp.x * m_iFboWidth;
				temp.y = temp.y * m_iFboHeight;

				if (temp.distance(ofVec2f(x, y)) < m_iRadiusClosePolyZone){
					m_vAreaPolygonsVector[m_iNumberOfAreaPolygons - 1].complete();
					m_bEditMode = false;
					if (m_vAreaPolygonsVector[m_iNumberOfAreaPolygons - 1].getSize() <= 2){
						m_vAreaPolygonsVector.pop_back();
					}
					isLastPoint = true;
					m_sPolyName = std::to_string(m_iNextFreeId);
				}
				//Create another point
				else{
					m_vAreaPolygonsVector[m_iNumberOfAreaPolygons - 1].addPoint(ofVec2f(static_cast<float>(x) / m_iFboWidth, static_cast<float>(y) / m_iFboHeight));
				}
			}

			//Every AreaPolygons are completed so create a new one
			else{
				m_vAreaPolygonsVector.push_back(AreaPolygon(ofVec2f(static_cast<float>(x) / m_iFboWidth, static_cast<float>(y) / m_iFboHeight), m_oPeople, m_sPolyName ,m_iAntiBounce));
				m_iNextFreeId++;
				m_bEditMode = true;
			}
		}

		//Selection
		if (!m_bEditMode && !isLastPoint){
			if(!m_bSelectMode){
				for (int i = 0; i < m_vAreaPolygonsVector.size(); i++){
					if (m_vAreaPolygonsVector[i].isPointInPolygon(ofPoint(static_cast<float>(x) / m_iFboWidth, static_cast<float>(y) / m_iFboHeight))){
						//We enter in selection mode
						m_iIndicePolygonSelected = i;
						m_vAreaPolygonsVector[i].hasBeenSelected(true);
						m_bSelectMode = true;
						m_sPolyName = m_vAreaPolygonsVector[i].getPolyName();
						break;//because we only want one selectd poly
					}
				}
			}
			else{				
				if (!m_vAreaPolygonsVector[m_iIndicePolygonSelected].isPointInPolygon(ofPoint(static_cast<float>(x) / m_iFboWidth, static_cast<float>(y) / m_iFboHeight))){
					m_sPolyName = std::to_string(m_iNextFreeId);
					//We leave the selection mode
					m_vAreaPolygonsVector[m_iIndicePolygonSelected].hasBeenSelected(false);
					m_iIndicePolygonSelected = -1;
					m_bSelectMode = false;

					//We change the selected poly
					for (int i = 0; i < m_vAreaPolygonsVector.size(); i++){
						if (m_vAreaPolygonsVector[i].isPointInPolygon(ofPoint(static_cast<float>(x) / m_iFboWidth, static_cast<float>(y) / m_iFboHeight))){						
							m_iIndicePolygonSelected = i;
							m_vAreaPolygonsVector[i].hasBeenSelected(true);
							m_sPolyName = m_vAreaPolygonsVector[i].getPolyName();
							m_bSelectMode = true;
							break;//because we only want one selectd poly
						}

					}
				}
			}
		}
	}
}



//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	
	ofPoint temp = transformMouseCoord(x, y);
	x = temp.x;
	y = temp.y;

	ofVec2f movement = ofVec2f(m_oOldMousePosition.x - x, m_oOldMousePosition.y - y);

	if (button == 0){
		if (!m_bEditMode){
			if (m_bSelectMode){
				m_vAreaPolygonsVector[m_iIndicePolygonSelected].move(static_cast<float>(movement.x) / m_iFboWidth, static_cast<float>(movement.y) / m_iFboHeight);
				m_vAreaPolygonsVector[m_iIndicePolygonSelected].setPolygonCentroid();
			}
		}
	}
	m_oOldMousePosition = ofVec2f(x, y);
}

//--------------------------------------------------------------
ofPoint ofApp::transformMouseCoord(int x, int y){
	x = x * m_iFboWidth / (m_iWidthRender * m_fZoomCoef);
	y = y * m_iFboHeight / (m_iHeightRender * m_fZoomCoef);
	return ofPoint(x, y);
}

//_______________________________________________________________
//_____________________________SAVE & LOAD_______________________
//_______________________________________________________________

//--------------------------------------------------------------
void ofApp::saveSettings(){    
    // Save GUI parameters
    m_gui.saveToFile("settings.xml");
	savePreferences(); 
}

//--------------------------------------------------------------
void ofApp::loadSettings(){
    // Load saved settings
    if(ofFile::doesFileExist("settings.xml")){
        m_gui.loadFromFile("settings.xml");
    }
	if (ofFile::doesFileExist("preferences.xml")){
		if (m_bSelectMode){
			m_vAreaPolygonsVector[m_iIndicePolygonSelected].hasBeenSelected(false);
			m_iIndicePolygonSelected = -1;
			m_bSelectMode = false;
		}
		m_vAreaPolygonsVector.clear();
		m_bEditMode = false;
		loadPreferences();
	}
}

//--------------------------------------------------------------
void ofApp::savePreferences(){

	ofxXmlSettings preferences;
	preferences.addTag("Settings");
	preferences.pushTag("Settings");
	preferences.addValue("HideInterface", m_bHideInterface);
	preferences.addValue("LogToFile", m_bLogToFile);
	preferences.addValue("FboWidth", m_iFboWidth);
	preferences.addValue("FboHeight", m_iFboHeight);
	preferences.addValue("NextFreeId", m_iNextFreeId);

	preferences.addTag("OSC");
	preferences.pushTag("OSC");
	preferences.setValue("ReceiverPort", m_sInputPort);
	preferences.setValue("SenderPort", m_sOutputPort);
	preferences.setValue("SenderHost", m_sOutputIp);
	preferences.popTag();

	for (int i = 0; i < m_iNumberOfAreaPolygons; i++){
		if (m_vAreaPolygonsVector[i].isCompleted()){

			preferences.addTag("AreaPolygon");
			preferences.pushTag("AreaPolygon", i);
			preferences.addValue("name", m_vAreaPolygonsVector[i].getPolyName());

			for (int j = 0; j < m_vAreaPolygonsVector[i].getSize(); j++){
				preferences.addTag("Point");
				preferences.pushTag("Point", j);
				preferences.addValue("x", m_vAreaPolygonsVector[i].getPoint(j).x);
				preferences.addValue("y", m_vAreaPolygonsVector[i].getPoint(j).y);
				preferences.popTag();
			}

			preferences.addTag("Osc");
			preferences.pushTag("Osc");
				preferences.addValue("In", m_vAreaPolygonsVector[i].getInOscAll());
				preferences.addValue("Out", m_vAreaPolygonsVector[i].getOutOscAll());
			preferences.popTag();
			preferences.popTag();
		}
	}
	preferences.popTag();

	if (!preferences.saveFile("preferences.xml"))
		ofLogVerbose("Failed to save file");
}

//--------------------------------------------------------------
void ofApp::loadPreferences(){
	ofxXmlSettings preferences;
	ofVec2f p;
	string name;
	int nbrPoints;
	int nbrPolygons;

	// If a preferences.xml file exists, load it
	if (ofFile::doesFileExist("preferences.xml")){
		preferences.load("preferences.xml");

		preferences.pushTag("Settings");

		if (preferences.tagExists("OSC")) {
			preferences.pushTag("OSC");
			m_sInputPort = preferences.getValue("ReceiverPort", m_sInputPort);
			m_sOutputPort = preferences.getValue("SenderPort", m_sOutputPort);
			m_sOutputIp = preferences.getValue("SenderHost", m_sOutputIp);
			preferences.popTag();
		}

		m_bHideInterface = preferences.getValue("HideInterface", m_bHideInterface);
		m_bLogToFile = preferences.getValue("LogToFile", m_bLogToFile);
		m_iFboWidth = preferences.getValue("FboWidth", m_iFboWidth);
		m_iFboHeight = preferences.getValue("FboHeight", m_iFboHeight);
		m_iNextFreeId = preferences.getValue("NextFreeId", m_iNextFreeId);

		nbrPolygons = preferences.getNumTags("AreaPolygon");
		ofLogVerbose("loadPreferences") << "load of " << nbrPolygons << " polygons";

		for (int i = 0; i < nbrPolygons; ++i){
			preferences.pushTag("AreaPolygon", i);
			name = preferences.getValue("name", "noName");
			nbrPoints = preferences.getNumTags("Point");
			ofLogVerbose("loadPreferences") << "load of " << name << " polygon with " << nbrPoints << " points";

			for (int j = 0; j < nbrPoints; j++){
				preferences.pushTag("Point", j);

				p.x = preferences.getValue("x", 0.0f);
				p.y = preferences.getValue("y", 0.0f);

				if (j == 0){
					m_vAreaPolygonsVector.push_back(AreaPolygon(ofVec2f(p.x, p.y), m_oPeople, name, m_iAntiBounce));

				}
				else{
					m_vAreaPolygonsVector[i].addPoint(ofVec2f(p.x, p.y));
				}

				ofLogVerbose("loadPreferences") << "x = " << p.x;
				ofLogVerbose("loadPreferences") << "y = " << p.y;

				preferences.popTag();
			}
			m_vAreaPolygonsVector[i].complete();

				preferences.pushTag("Osc");
				m_vAreaPolygonsVector[i].loadOscMessage(preferences.getValue("In", "/area/" + ofToString(m_iNextFreeId) + "/personEntered"),
														preferences.getValue("Out", "/area/" + ofToString(m_iNextFreeId) + "/personWillLeave"));
				preferences.popTag();

			preferences.popTag();
		}
		preferences.popTag();

		if (m_iNextFreeId < m_vAreaPolygonsVector.size()){
			ofLogError("Problem detected in the naming of the polygons..");
		}
	}
	else{
		ofFile newFile(ofToDataPath("preferences.xml"), ofFile::WriteOnly); //file doesn't exist yet
		newFile.create();
	}
}

//_______________________________________________________________
//_____________________________OSC_______________________________
//_______________________________________________________________

//--------------------------------------------------------------
void ofApp::sendOSC(){
// Create a new message
ofxOscMessage m;
	if (m_bRedondanteMode){	
		for (int i = 0; i < m_iNumberOfAreaPolygons; ++i){
			if (m_vAreaPolygonsVector[i].isCompleted()){
				if (m_vAreaPolygonsVector[i].getPeopleMovement() > 0){		
					// Create a first dataset
					m = m_vAreaPolygonsVector[i].getInOscMessage();
					m.setAddress(m_vAreaPolygonsVector[i].getInOscAdress());
					for (int j = 0; j < m_vAreaPolygonsVector[i].getPeopleMovement(); j++){
						m.addInt32Arg(m_vAreaPolygonsVector[i].getLastIdWhichEntered());
						m_oscSender.sendMessage(m); // Send it
						ofLogVerbose("sendOSC") << m_vAreaPolygonsVector[i].getInOscAdress();
					}
					m.clear(); // Clear message to be able to reuse it
				}
				if (m_vAreaPolygonsVector[i].getPeopleMovement() < 0){
					// Create a second dataset
					m = m_vAreaPolygonsVector[i].getOutOscMessage();
					m.setAddress(m_vAreaPolygonsVector[i].getOutOscAdress());
					for (int j = 0; j < abs(m_vAreaPolygonsVector[i].getPeopleMovement()); j++){
						m.addInt32Arg(m_vAreaPolygonsVector[i].getLastIdWhichLeft());
						m_oscSender.sendMessage(m); // Send it
						ofLogVerbose("sendOSC") << m_vAreaPolygonsVector[i].getOutOscAdress();
					}
					m.clear(); // Clear message to be able to reuse it
				}
			}
		}
	}
	else{//Non redondant mode 
		for (int i = 0; i < m_iNumberOfAreaPolygons; ++i){
			if (m_vAreaPolygonsVector[i].isCompleted()){
				//We can work on this polygon
				if (m_vAreaPolygonsVector[i].getPeopleInsideCount() == 0 
					&& m_vAreaPolygonsVector[i].getPeopleMovement() < 0){
					//X->0
					m = m_vAreaPolygonsVector[i].getOutOscMessage();
					m.setAddress(m_vAreaPolygonsVector[i].getOutOscAdress());
					m.addInt32Arg(m_vAreaPolygonsVector[i].getLastIdWhichLeft());
					m_oscSender.sendMessage(m); // Send it
					m.clear(); // Clear message to be able to reuse it
					ofLogVerbose("sendOSC") << m_vAreaPolygonsVector[i].getOutOscAdress();
				}
				if (m_vAreaPolygonsVector[i].getPeopleInsideCount() != 0 
					&& m_vAreaPolygonsVector[i].getPeopleInsideCount() == m_vAreaPolygonsVector[i].getPeopleMovement()){
					//0->X
					m = m_vAreaPolygonsVector[i].getInOscMessage();
					m.setAddress(m_vAreaPolygonsVector[i].getInOscAdress());
					m.addInt32Arg(m_vAreaPolygonsVector[i].getLastIdWhichEntered());
					m_oscSender.sendMessage(m); // Send it
					m.clear(); // Clear message to be able to reuse it
					ofLogVerbose("sendOSC") << m_vAreaPolygonsVector[i].getInOscAdress();
				}
			}
		}
	}
}

//_______________________________________________________________
//_____________________________EXIT_______________________________
//_______________________________________________________________

//--------------------------------------------------------------
void ofApp::exit(){

#ifdef WIN32
	m_spoutSender.exit();
#endif

	/*
	Save parameters set in GUI before closing app
	Parameters are saved in another file that the one used in saveSettings() function
	to prevent saving wrong parameters if application quit unexpectedly.
	*/
	m_sPolyName = std::to_string(m_iNextFreeId);
	m_gui.saveToFile("autosave.xml");
	savePreferences();
	// Remove listener because instance of our gui button will be deleted
	m_bResetSettings.removeListener(this, &ofApp::reset);
}
