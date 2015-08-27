#ifndef AreaPolygon_H
#define AreaPolygon_H

#include <vector>
#include "ofVec2f.h"
#include "ofGraphics.h"
#include "ofxAugmenta.h"

class LeavingPerson{

public:
	LeavingPerson(unsigned long long a_ullTime,int a_iId){
		m_ulLeavingTime = a_ullTime;
		m_iId = a_iId;
	}

	inline unsigned long long getLeavingTime(){ return m_ulLeavingTime; };
	inline int getId(){ return m_iId; };

private:
	unsigned long long m_ulLeavingTime;
	int m_iId;

};

class AreaPolygon {
public:

	//Construction
	AreaPolygon(ofVec2f a_oFirstPoint, vector<Augmenta::Person*> a_vPeople, int a_iIndice, int m_iAntiBounce);

	//Inline
	inline bool isCompleted(){ return m_bIsFinished; };
	inline int getSize(){ return m_vVectorPoints.size(); };
	inline ofVec2f getPoint(int a_iIndice){ return m_vVectorPoints[a_iIndice]; };
	inline void hasBeenSelected(bool a_bSelected){ m_bSelected = a_bSelected; };
	inline int getPeopleMovement(){ return m_iPeopleMovement; };
	inline int getPeopleInside(){ return m_iPeopleInside; };

	//Osc
	inline string getInOscAdress(){ return m_vInOsc[0]; };
	inline string getOutOscAdress(){ return m_vOutOsc[0]; };
	inline ofxOscMessage getInOscMessage(){ return m_oOscMessageIn; };
	inline ofxOscMessage getOutOscMessage(){ return m_oOscMessageOut; };
	string getInOscAll(); 
	string getOutOscAll();
	void loadOscMessage(string m_aInOsc, string m_aOutOsc);
		
	//Draw
	void draw(int width, int height); 
	void drawPeopleInside(int width, int height);
	void drawPeopleMovement(int width, int height);

	//Move Poly
	void moveLeft();
	void moveRight();
	void moveUp();
	void moveDown();
	void move(float a_iX, float a_iY);

	void update(vector<Augmenta::Person*> a_vPeople, int a_iBounceIntervalTime);
	void setPeopleInside(vector<Augmenta::Person*> a_vPeople, int a_iBounceIntervalTime);
	void setPolygonCentroid();
	void complete();
	void addPoint(ofVec2f a_oPoint);
	bool removeLastPoint();
	bool isPointInPolygon(ofVec2f a_oPersonPosition);
	bool doesStringContainOnlyNumber(string a_sString);
	bool containOnlyAlpha(string s);
	bool doesStringContainOnlyNumberAndOnePoint(string s);

private:

	vector<ofVec2f> m_vVectorPoints;
	bool m_bIsFinished;
	int m_iPeopleInside;
	int m_iOldPeopleInside;
	bool m_bSelected;
	ofVec2f m_oCentroid;
	int m_iPeopleMovement;
	vector<string> m_vInOsc;
	vector<string> m_vOutOsc;
	ofxOscMessage m_oOscMessageIn;
	ofxOscMessage m_oOscMessageOut;
	float m_fMoveIncremente;
	vector<LeavingPerson> m_vWaitingToLeave;
	vector<int> m_vIdPeopleInside;

	//Design
	ofColor m_oPointsColor;
	ofColor m_oLinesColor;
	ofColor m_oCompletedColor;
	ofColor m_oLinesCompletedColor;
	ofColor m_oNotEmptyColor;
	float m_fRadius;
	int m_iLinesWidth;

};
#endif // AreaPolygon_H
