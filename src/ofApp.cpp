#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    // Set the log level
    ofSetLogLevel("detector", OF_LOG_NOTICE);
    ofLogToConsole();
    
    // Set initial camera dimensions
    camWidth = 640;
    camHeight = 480;
    camAspect = (float)camWidth / (float)camHeight;
    
    //Check for hi resolution display
    int guiMultiply = 1;
    if (ofGetScreenWidth() >= RETINA_MIN_WIDTH) {
        guiMultiply = 2;
    }
    
    //Window size based on screen dimensions, centered
    ofSetWindowShape((int)ofGetScreenHeight() / 2 * camAspect + (200 * guiMultiply), (int)ofGetScreenHeight()*0.9);
    ofSetWindowPosition((ofGetScreenWidth() / 2) - ofGetWindowWidth() / 2, ((int)ofGetScreenHeight() / 2) - ofGetWindowHeight() / 2);
    
    int framerate = 30; // Used to set oF and camera framerate
    ofSetFrameRate(framerate);
    ofBackground(ofColor::black);
    ofSetWindowTitle("LightWork");
    
    // Mapping
    isMapping = false;
    
    //Fbos
    camFbo.allocate(camWidth, camHeight);
    camFbo.begin();
    ofClear(255, 255, 255);
    camFbo.end();

    //Video Devices
    devices = cams[0].listDevices(); 
    for (int i=0; i < devices.size(); i++) {
        cams[i].setVerbose(false);
        cams[i].setDeviceID(i); // Default to external camera
        cams[i].setPixelFormat(OF_PIXELS_RGB);
        cams[i].setup(camWidth, camHeight);
    }
    camPtr = &cams[1];
    
    
    // Tracking
    isMapping = false;

    // Connect to the fcserver
    IP = "192.168.1.104"; //Default IP for Fadecandy
    opcClient.setup(IP, 7890, 1);
    opcClient.setLedsPerStrip(50); //TODO: Use GUI Variable
    opcClient.setInterpolation(false);

    // Animator settings
    animator.setLedInterface(&opcClient); // Setting a POINTER to the interface, so the Animator class can update pixels internally
    animator.setMode(ANIMATION_MODE_CHASE);
    animator.setNumLedsPerStrip(50); // This also updates numLedsPerStrip in the OPC Client
    animator.setNumStrips(4); // TODO: Fix setNumStrips, it gets set to n-1
    
    animator.setFrameSkip(5);
    animator.setAllLEDColours(ofColor(0, 0, 0)); // Clear the LED strips
    animator.ledBrightness.set("LED Brightness", 102);
    animator.rOffset.set("Red Offset", 0);
    animator.gOffset.set("Green Offset", 16);
    animator.bOffset.set("Blue Offset", -10);

    detector.setup(*camPtr);
    detector.setMode(DETECTOR_MODE_OFF);
    detector.learningTime.set("Learning Time", 4, 0, 30);
    detector.thresholdValue.set("Threshold Value", 30, 0, 255);
    
    detector.binaryBrightnessThreshold.setMin(0.0);
    detector.binaryBrightnessThreshold.setMax(1.0);
    detector.blueGreenDistanceThreshold.setMin(0.0);
    detector.blueGreenDistanceThreshold.setMax(1.0);
    detector.binaryBrightnessThreshold.set("Binary Brightness Threshold", 0.6);
    detector.blueGreenDistanceThreshold.set("Blue/Green Distance Threshold", 0.1);

    // SVG
    svg.setViewbox(0, 0, camWidth, camHeight);

    //GUI
    buildUI(guiMultiply);
}

//--------------------------------------------------------------
void ofApp::update() {
	opcClient.update();

	// If the client is not connected do not try to send information
	if (!opcClient.isConnected()) {
		// Will continue to try connecting to the OPC Pixel Server
		opcClient.tryConnecting();
	}

	camPtr->update();
    
    if (animator.mode == ANIMATION_MODE_TEST) {
        animator.update(); // Update the pixel values
        detector.update();
    }
    
    else if (animator.mode == ANIMATION_MODE_BINARY && isMapping) { // Redundant, for  now...
        // Update LEDs and Detector
        animator.update();
        detector.update();
        
        // Match detected patterns with known patterns
        for (auto it = detector.dict.begin(); it != detector.dict.end(); it++) {
           // ofLogVerbose("match") << "iterating over detector label: " << it->first;
            string detected = it->second.first.binaryPatternString;
            for (auto jit = animator.leds.begin(); jit != animator.leds.end(); jit++) {
                //ofLogVerbose("match") << "checking match for led at index: " << jit->address;
                string known = jit->binaryPattern.binaryPatternString;
                if (detected == known && !jit->hasFoundMatch) {
                    // Assign coordinates to LED
                    jit->setCoord(it->second.second);
                    ofLogVerbose("match") << "LED Address: " << jit->address << " coord: " << jit->coord << " pattern: " << jit->binaryPattern.binaryPatternString;
                }
            }
        }
        // Try to do this without the dictionary
        ofxCv::RectTracker tracker = detector.getTracker();
        
        vector <unsigned int> labels = tracker.getCurrentLabels();
        detector.getBoundingRects();
        
        
//        for (const *it = tracker.getCurrentLabels().begin(); it != tracker.getCurrentLabels().end(); it++) {
//            cout << "tracker label: " << it << endl;
//            
//        }
        
        // Check if all LEDs have a match
        
        for (auto it = animator.leds.begin(); it != animator.leds.end(); it++) {
            if (it->hasFoundMatch) {
                ofLogVerbose("match") << "LED Address: " << it->address << " coord: " << it->coord;
            }
        }
    }
    
    // New camera frame: Turn on a new LED and detect the location.
    // We are getting every third camera frame (to give the LEDs time to light up and the camera to pick it up).

    else if(camPtr->isFrameNew() && (animator.mode == ANIMATION_MODE_CHASE) && isMapping && (ofGetFrameNum()%5 == 0)) {
        // Make sure you call animator.update() once when you activate CHASE mode
        // We check if the tracker has found the first contour before processing with the animation
        // This makes sure we don't miss the first LED
        if (detector.hasFoundFirstContour) {
            animator.update();
        }
        detector.update();
    }
    
    // Only update the view, don't do any detection
    if (!isMapping) {
        detector.updateViewOnly();
    }
    
    ofSetColor(ofColor::white);
}

//--------------------------------------------------------------
void ofApp::draw(){
	//Draw into Fbo to allow scaling regardless of camera resolution
	camFbo.begin();
	camPtr->draw(0,0);

    ofSetColor(0, 255, 0);
	detector.draw(); // Draws the blob rect surrounding the contour
    
    
    // Draw labels and detected patterns
    ofSetColor(0, 0, 0);
    for (auto it = detector.dict.begin(); it!=detector.dict.end(); ++it) {
        int label = it->first;
        string patternString = it->second.first.binaryPatternString;
        ofPoint point = it->second.second;
        ofDrawBitmapString(ofToString(label), point.x+10, point.y);
        //ofDrawBitmapString(patternString, point.x+50, point.y);
    }

    
    // Draw the detected contour center points
    for (int i = 0; i < detector.centroids.size(); i++) {
		ofDrawCircle(detector.centroids[i].x, detector.centroids[i].y, 3);
    }
    
    // Draw detected binary pattern coordinates
    ofSetColor(255, 0, 0);
    for (auto it = animator.leds.begin(); it != animator.leds.end(); it++) {
        ofDrawCircle(it->coord.x, it->coord.y, 5);
    }
    
	camFbo.end();

	ofSetColor(ofColor::white); //reset color, else it tints the camera

	//Draw Fbo and Thresholding images to screen

	camFbo.draw(0, 0, (ofGetWindowHeight() / 2)*camAspect, ofGetWindowHeight()/2);
	//if (detector.thresholded.isAllocated()) {
		detector.thresholded.draw(0, ofGetWindowHeight() / 2, (ofGetWindowHeight() / 2)*camAspect, ofGetWindowHeight()/2);
    //}
    
    

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    switch (key){
        case ' ':
            detector.centroids.clear();
            break;
        case 's':
            detector.setMode(DETECTOR_MODE_CHASE);
            detector.centroids.clear();
            isMapping = !isMapping;
            animator.setMode(ANIMATION_MODE_CHASE);
            animator.setFrameSkip(3);
            animator.update();
            break; 
        case 'b':
            detector.setMode(DETECTOR_MODE_BINARY);
            detector.centroids.clear();
            isMapping = !isMapping;
            animator.setMode(ANIMATION_MODE_BINARY);
            animator.setFrameSkip(4);
            animator.setAllLEDColours(ofColor(0,0,0));
            if (isMapping) {
                animator.update();
            }
            break;
        case 'g':
            generateSVG(detector.centroids);
            break;
        case 'j':
            generateJSON(detector.centroids);
            break;
		case 't':
            detector.setMode(DETECTOR_MODE_OFF);
            isMapping = false;
			if (animator.mode == ANIMATION_MODE_TEST) { animator.setMode(ANIMATION_MODE_OFF); }
			else { animator.setMode(ANIMATION_MODE_TEST); }
            animator.update();
			break;
        case 'f': // filter points
            detector.centroids = removeDuplicatesFromPoints(detector.centroids);
            break;
        case 'e':
            vector <ofPoint> pts;
            for (int i = 0; i < animator.leds.size(); i++) {
                pts.push_back(animator.leds[i].coord);
            }
            
            //pts = removeDuplicatesFromPoints(pts);
            
            generateSVG(pts);
            break;
    }

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
	ofBackground(0, 0, 0);
	/*buildUI();*/

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

void ofApp::exit(){
	// Close Connection
	opcClient.close();
}

void ofApp::generateSVG(vector <ofPoint> points) {
	if (points.size() == 0) {
		//User is trying to save without anything to output - bail
		ofLogError("No point data to save, run mapping first");
		return;
	}
	
	ofPath path;
    for (int i = 0; i < points.size(); i++) {
        // Avoid generating a moveTo AND lineTo for the first point
        // If we don't specify the first moveTo message then the first lineTo will also produce a moveTo point with the same coordinates
        if (i == 0) {
            path.moveTo(points[i]);
        }
        else {
           path.lineTo(points[i]);
        }
        ofLogVerbose("output") << points[i].x << ", "<< points[i].y;
    }
    svg.addPath(path);
    path.draw();
//	if (detector.centroids.size() == 0) {
//		//User is trying to save without anything to output - bail
//		ofLogError("No point data to save, run mapping first");
//		return;
//	}

	ofFileDialogResult saveFileResult = ofSystemSaveDialog("layout.svg", "Save layout file");
	if (saveFileResult.bSuccess) {
		svg.save(saveFileResult.filePath);
        ofLogNotice("output") << "Saved SVG file.";
	}
}

void ofApp::generateJSON(vector<ofPoint> points) {
    int maxX = ofToInt(svg.info.width);
    int maxY = ofToInt(svg.info.height);
    ofLogNotice("output") << "maxX, maxY: " << maxX << ", " << maxY;
	//ofLogNotice() << maxX;
	//ofLogNotice() << maxY;
    
    ofxJSONElement json; // For output
    
    for (int i = 0; i < points.size(); i++) {
        Json::Value event;
        Json::Value vec(Json::arrayValue);
        vec.append(Json::Value(points[i].x/maxX)); // Normalized
        vec.append(Json::Value(0.0)); // Normalized
        vec.append(Json::Value(points[i].y/maxY));
        event["point"]=vec;
        json.append(event);
    }
    
    json.save("testLayout.json");
    ofLogNotice("output") << "Saved JSON file.";
}


/*
 I'm expecting a series of 2D points. I need to filter out points that are too close together, but keep
 negative points. The one that are negative represent 'invisible' or 'skipped' LEDs that have a physical presence
 in an LED strip but are not visuable. We need to store them 'off the canvas' so that our client application (Lightwork Scraper) can be aware of the missing LEDs (as they are treated sequentially, with no 'fixed' address mapping.
 IDEA: Can we store the physical address as the 'z' in a Vec3 or otherwise encode it in the SVG. Maybe we can make another
 'path' in the SVG that stores the address in a path of the same length.
 */
vector <ofPoint> ofApp::removeDuplicatesFromPoints(vector <ofPoint> points) {
    ofLogNotice("tracking") << "Removing duplicates";
    
    float thresh = 3.0; // TODO: Add interface to GUI
    
    std::vector<ofPoint>::iterator iter;
    
    // Iterate through all the points and remove duplicates and 'extra' points (under threshold distance).
    for (iter = points.begin(); iter < points.end(); iter++) {
        int i = std::distance(points.begin(), iter); // Index of iter, used to avoid comporating a point to itself
        ofPoint pt = *iter;
        ofLogVerbose("tracking") << "BASE: " << pt << endl;
        
        // Do not remove 0,0 points (they're 'invisible' LEDs, we need to keep them).
        if (pt.x == 0 && pt.y == 0) {
            continue; // Go to the next iteration
        }
        
        // Compare point to all other points
        std::vector<ofPoint>::iterator j_iter;
        for (j_iter = points.begin(); j_iter < points.end(); j_iter++) {
            int j = std::distance(points.begin(), j_iter); // Index of j_iter
            ofPoint pt2 = *j_iter;
            ofLogVerbose("tracking") << "NESTED: " << pt2 << endl;
            float dist = pt.distance(pt2);
            ofLogVerbose("tracking") << "DISTANCE: " << dist << endl;
            ofLogVerbose("tracking") << "i: " << i << " j: " << j << endl;
            // Comparing point to itself... do nothing and move on.
            if (i == j) {
                ofLogVerbose("tracking") << "COMPARING POINT TO ITSELF " << pt << endl;
                continue; // Move on to the next j point
            }
            // Duplicate point detection. (This might be covered by the distance check below and therefor redundant...)
            else if (pt.x == pt2.x && pt.y == pt2.y) {
                ofLogVerbose("tracking") << "FOUND DUPLICATE POINT (that is not 0,0) - removing..." << endl;
                iter = points.erase(iter);
                break;
            }
            // Check point distance, remove points that are too close
            else if (dist < thresh) {
                ofLogVerbose("tracking") << "REMOVING" << endl;
                iter = points.erase(iter);
                break;
            }
        }
    }
    
    return points;
}

//Dropdown Handler
void ofApp::onDropdownEvent(ofxDatGuiDropdownEvent e)
{
	if (e.target->is("Select Camera")) {
		vector<string> devices = enumerateCams();
		switchCamera(e.child, camWidth, camHeight);
		//TODO - figure out how to repopulate the list
		//ofLogNotice() << "Camera " << e.child << " was selected";
		guiBottom->getLabel("Message Area")->setLabel((gui->getDropdown("Select Camera")->getChildAt(e.child)->getLabel())+" selected");
		int guiMultiply = 1;
		if (ofGetScreenWidth() >= RETINA_MIN_WIDTH) {
			guiMultiply = 2;
		}
		ofSetWindowShape((int)ofGetScreenHeight() / 2 * camAspect + (200 * guiMultiply), (int)ofGetScreenHeight()*0.9);
		gui->update();
	}

	if (e.target->is("Select Driver Type")) {
		if (e.child == 0) {
			ofLogNotice() << "Pixel Pusher was selected";
			guiBottom->getLabel("Message Area")->setLabel("Pixel Pusher selected");
			gui->getFolder("PixelPusher Settings")->setVisible(true);
			gui->getFolder("PixelPusher Settings")->expand();
			gui->getFolder("Mapping Settings")->setVisible(true);
			gui->getFolder("Mapping Settings")->expand();
			gui->getFolder("Fadecandy Settings")->setVisible(false);
			gui->getFolder("Fadecandy Settings")->collapse();
		}
		else if (e.child == 1) {
			ofLogNotice() << "Fadecandy/Octo was selected";
			guiBottom->getLabel("Message Area")->setLabel("Fadecandy/Octo selected");
			gui->getFolder("Fadecandy Settings")->setVisible(true);
			gui->getFolder("Fadecandy Settings")->expand();
			gui->getFolder("Mapping Settings")->setVisible(true);
			gui->getFolder("Mapping Settings")->expand();
			gui->getFolder("PixelPusher Settings")->setVisible(false);
			gui->getFolder("PixelPusher Settings")->collapse();
		}
	}
}

//GUI event handlers
void ofApp::onSliderEvent(ofxDatGuiSliderEvent e)
{
	//while (!ofGetMousePressed(OF_MOUSE_BUTTON_LEFT)) {
	ofLogNotice() << "onSliderEvent: " << e.target->getLabel() << " "; e.target->printValue(); //TODO: stop from spamming output
	////if (e.target->is("gui opacity")) gui->setOpacity(e.scale);
	//}

}

void ofApp::onTextInputEvent(ofxDatGuiTextInputEvent e)
{
	ofLogNotice("gui") << "onTextInputEvent: " << e.target->getLabel() << " " << e.target->getText();

	if (e.target->is("IP")) {
		IP= e.target->getText();
		if (opcClient.isConnected()) {
			opcClient.close();
			if (!opcClient.isConnected()) { gui->getLabel("Connection Status", "Fadecandy Settings")->setLabel("Disconnected"); }
		}
		if (!opcClient.isConnected()) {
			opcClient.setup(IP, 7890);
			if (opcClient.isConnected()) { gui->getLabel("Connection Status", "Fadecandy Settings")->setLabel("Connected"); }
		}
	}

	if (e.target->is("LEDS per Strip")) {
		string temp = e.target->getText();
        animator.setNumLedsPerStrip(ofToInt(temp));
	}

	if (e.target->is("STRIPS")) {
		string temp = e.target->getText();
		animator.setNumStrips(ofToInt(temp));
	}
}

void ofApp::onButtonEvent(ofxDatGuiButtonEvent e)
{
	ofLogNotice("gui") << "onButtonEvent: " << e.target->getLabel();

	if (e.target->is("TEST LEDS")) {
		if (animator.mode == ANIMATION_MODE_TEST) { 
			animator.setMode(ANIMATION_MODE_OFF); 
			animator.update();
		}
		else { 
			animator.setMode(ANIMATION_MODE_TEST); 
			animator.update();
		}
//        opcClient.autoWriteData(animator.getPixels());
	}
	if (e.target->is("MAP LEDS")) {
        detector.setMode(DETECTOR_MODE_CHASE);
        
        isMapping = !isMapping;
        animator.setMode(ANIMATION_MODE_CHASE);
        animator.setFrameSkip(3);
        if (!isMapping) {
            detector.centroids.clear();
            animator.setAllLEDColours(ofColor(0,0,0));
        }
        else {
            animator.update();
        }
        

	}
	if (e.target->is("SAVE LAYOUT")) {
		detector.centroids = removeDuplicatesFromPoints(detector.centroids);
		generateSVG(detector.centroids);
	}

}

//Used to change acctive camera during runtime. Necessary to close old camera first before initializing the new one.
void ofApp::switchCamera(int num, int w, int h)
{
	//ofLogNotice("Switching camera");
	//if(cam.isInitialized()){
	//	cam.close();
	//	//ofLogNotice() << cam.isInitialized();
	//	cam2.setDeviceID(num);
	//	cam2.setPixelFormat(OF_PIXELS_RGB);
	//	cam2.setup(w, h);
	//	camPtr = &cam2;
	//}	
	//
	//else if (cam2.isInitialized()) {
	//	cam2.close();
	//	cam.setDeviceID(num);
	//	cam.setPixelFormat(OF_PIXELS_RGB);
	//	cam.setup(w, h);
	//	camPtr = &cam;
	//}

	camPtr = &cams[num];
    detector.setup(*camPtr);

}
//Returns a vector containing all the attached cameras
vector<string> ofApp::enumerateCams()
{
	devices.clear();
	devices = cams[0].listDevices();
	vector<string> deviceStrings;

	for (std::vector<ofVideoDevice>::iterator it = devices.begin(); it != devices.end(); ++it) {
		int i = std::distance(devices.begin(), it);
		ofVideoDevice device = *it;
		string name = device.deviceName;
		int id = device.id;
		//ofLogNotice() << "Camera " << id << ": " <<  name << endl;
		deviceStrings.push_back(name);

	}
		return deviceStrings;
}

void ofApp::buildUI(int mult)
{
	//GUI
	gui = new ofxDatGui(ofxDatGuiAnchor::TOP_RIGHT);
	guiBottom = new ofxDatGui(ofxDatGuiAnchor::BOTTOM_RIGHT);
	//gui->setTheme(new ofxDatGuiThemeSmoke());
	//gui->addHeader(":: drag me to reposition ::");

	gui->addDropdown("Select Camera", enumerateCams());
	gui->addBreak();

	vector<string> opts = { "PixelPusher", "Fadecandy/Octo" };
	gui->addDropdown("Select Driver Type", opts);
	gui->addBreak();

	ofxDatGuiFolder* fcSettings = gui->addFolder("Fadecandy Settings", ofColor::white);
	fcSettings->addTextInput("IP", IP);

	fcSettings->addTextInput("STRIPS", ofToString(animator.getNumStrips()));
	fcSettings->addTextInput("LEDS per Strip", ofToString(animator.getNumLedsPerStrip()));
	
	string connection;
	if (opcClient.isConnected()) {
		connection = "connected";
		fcSettings->addLabel("Connection Status");
		gui->getLabel("Connection Status")->setLabel(connection);
	}
	else {
		connection = "disconnected";
		fcSettings->addLabel("Connection Status");
		gui->getLabel("Connection Status")->setLabel(connection);
	}
	fcSettings->setVisible(false);
	fcSettings->addBreak();
	
	ofxDatGuiFolder* ppSettings = gui->addFolder("PixelPusher Settings", ofColor::white);
	ppSettings->addTextInput("IP", IP);
	ppSettings->addTextInput("STRIPS", ofToString(animator.getNumStrips()));
	ppSettings->addTextInput("LEDS per Strip", ofToString(animator.getNumLedsPerStrip()));
	ppSettings->setVisible(false);
	ppSettings->addBreak();


	

	ofxDatGuiFolder* mapSettings = gui->addFolder("Mapping Settings", ofColor::dimGrey);
	mapSettings->addSlider(detector.learningTime);
	mapSettings->addSlider(detector.thresholdValue);
	mapSettings->addButton("Test LEDS");
	mapSettings->addButton("Map LEDS");
	mapSettings->addButton("Save Layout");
	mapSettings->setVisible(false);
	mapSettings->addBreak();
    
    mapSettings->addSlider(animator.ledBrightness);
    mapSettings->addSlider(animator.rOffset);
    mapSettings->addSlider(animator.gOffset);
    mapSettings->addSlider(animator.bOffset);
    
    mapSettings->addSlider(detector.binaryBrightnessThreshold);
    mapSettings->addSlider(detector.blueGreenDistanceThreshold);
    
    

	//Program Status GUI
	//guiBottom->addSlider("gui opacity", 0, 100, 50);
	guiBottom->addLabel("Message Area");
	guiBottom->addFRM()->setAnchor(ofxDatGuiAnchor::BOTTOM_RIGHT);
	//guiBottom->onSliderEvent(this, &ofApp::onSliderEvent);
	//guiBottom->

	//gui->addFooter();
	//gui->setOpacity(gui->getSlider("gui opacity")->getScale());

	// once the gui has been assembled, register callbacks to listen for component specific events //
	gui->onButtonEvent(this, &ofApp::onButtonEvent);
	//gui->onToggleEvent(this, &ofApp::onToggleEvent);
	gui->onSliderEvent(this, &ofApp::onSliderEvent);
	gui->onTextInputEvent(this, &ofApp::onTextInputEvent);
	//gui->on2dPadEvent(this, &ofApp::on2dPadEvent);
	gui->onDropdownEvent(this, &ofApp::onDropdownEvent);
	//gui->onColorPickerEvent(this, &ofApp::onColorPickerEvent);
	//gui->onMatrixEvent(this, &ofApp::onMatrixEvent);
}
