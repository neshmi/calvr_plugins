#include "GroupedBarGraph.h"

#include <cvrKernel/CalVR.h>
#include <cvrConfig/ConfigManager.h>

#include <iostream>
#include <sstream>
#include <cfloat>
#include <cmath>

using namespace cvr;

GroupedBarGraph::GroupedBarGraph(float width, float height)
{
    _width = width;
    _height = height;

    _font = osgText::readFontFile(CalVR::instance()->getHomeDir() + "/resources/arial.ttf");

    _topPaddingMult = 0.1;
    _leftPaddingMult = 0.1;
    _maxBottomPaddingMult = _currentBottomPaddingMult = 0.25;
}

GroupedBarGraph::~GroupedBarGraph()
{
}

bool GroupedBarGraph::setGraph(std::string title, std::map<std::string, std::vector<std::pair<std::string, float> > > & data, std::vector<std::string> & groupOrder, BarGraphAxisType axisType, std::string axisLabel, std::string axisUnits, std::string groupLabel, osg::Vec4 color)
{
    _title = title;
    _axisLabel = axisLabel;
    _axisUnits = axisUnits;
    _groupLabel = groupLabel;
    _axisType = axisType;
    _color = color;

    // bad, but for the moment I will assume the data sets to be small enough
    _data = data;
    _groupOrder = groupOrder;

    float minValue = FLT_MAX;
    float maxValue = FLT_MIN;

    _numBars = 0;

    std::map<std::string, std::vector<std::pair<std::string, float> > >::iterator it;
    for(it = _data.begin(); it != _data.end(); it++)
    {
	for(int i = 0; i < it->second.size(); ++i)
	{
	    if(it->second[i].second < minValue)
	    {
		minValue = it->second[i].second;
	    }
	    if(it->second[i].second > maxValue)
	    {
		maxValue = it->second[i].second;
	    }
	    _numBars++;
	}
    }

    _minGraphValue = minValue;
    _maxGraphValue = maxValue;

    float lowerPadding = 0.05;

    switch(axisType)
    {
	case BGAT_LINEAR:
	{
	    _maxDisplayRange = _maxGraphValue;
	    _minDisplayRange = _minGraphValue - lowerPadding * (_maxGraphValue - _minGraphValue);
	    break;
	}
	case BGAT_LOG:
	{
	    float logMax = log10(_maxGraphValue);
	    logMax = ceil(logMax);
	    _maxDisplayRange = pow(10.0,logMax);

	    float logMin = log10(_minGraphValue);
	    logMin = logMin - lowerPadding * (logMax - logMin);
	    _minDisplayRange = pow(10.0,logMin);
	    break;
	}
	default:
	{
	    _minDisplayRange = _minGraphValue;
	    _maxDisplayRange = _maxGraphValue;
	    break;
	}
    }

    if(!_root)
    {
	_root = new osg::Group();
	_bgScaleMT = new osg::MatrixTransform();
	_barGeode = new osg::Geode();
	_axisGeode = new osg::Geode();
	_bgGeode = new osg::Geode();

	_bgScaleMT->addChild(_bgGeode);
	_root->addChild(_bgScaleMT);
	_root->addChild(_barGeode);
	_root->addChild(_axisGeode);
    }
    else
    {
	//TODO: clear existing geometry
    }

    osg::StateSet * stateset = _root->getOrCreateStateSet();
    stateset->setMode(GL_LIGHTING,osg::StateAttribute::OFF);

    makeBG();
    makeGraph();

    update();

    return true;
}

void GroupedBarGraph::makeGraph()
{
    _barGeom = new osg::Geometry();

    osg::Vec3Array * verts = new osg::Vec3Array();
    osg::Vec4Array * colors = new osg::Vec4Array(1);
    colors->at(0) = _color;

    _barGeom->setVertexArray(verts);
    _barGeom->setColorArray(colors);
    _barGeom->setColorBinding(osg::Geometry::BIND_OVERALL);

    std::map<std::string, std::vector<std::pair<std::string, float> > >::iterator it = _data.begin();
    for(;it != _data.end(); it++)
    {
	for(int i = 0; i < it->second.size(); ++i)
	{
	    verts->push_back(osg::Vec3(0,0,0));
	    verts->push_back(osg::Vec3(0,0,0));
	    verts->push_back(osg::Vec3(0,0,0));
	    verts->push_back(osg::Vec3(0,0,0));
	}
    }

    _barGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,verts->size()));
    _barGeode->addDrawable(_barGeom);
}

void GroupedBarGraph::makeBG()
{
    osg::Geometry * geom = new osg::Geometry();

    osg::Vec3Array * verts = new osg::Vec3Array(4);
    verts->at(0) = osg::Vec3(0.5,1,0.5);
    verts->at(1) = osg::Vec3(0.5,1,-0.5);
    verts->at(2) = osg::Vec3(-0.5,1,0.5);
    verts->at(3) = osg::Vec3(-0.5,1,-0.5);

    osg::Vec4Array * colors = new osg::Vec4Array(1);
    colors->at(0) = osg::Vec4(1.0,1.0,1.0,1.0);

    geom->setColorArray(colors);
    geom->setColorBinding(osg::Geometry::BIND_OVERALL);
    geom->setVertexArray(verts);
    geom->setUseDisplayList(false);

    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_STRIP,0,4));

    _bgGeode->addDrawable(geom);
}

void GroupedBarGraph::update()
{
    // update bg scale
    osg::Vec3 scale(_width,1.0,_height);
    osg::Matrix scaleMat;
    scaleMat.makeScale(scale);
    _bgScaleMT->setMatrix(scaleMat);

    updateAxis();
    updateGraph();
}

void GroupedBarGraph::updateGraph()
{
    float graphLeft = _width * (_leftPaddingMult - 0.5);
    float graphBottom = _height * (_currentBottomPaddingMult - 0.5);
    float graphTop = _height * (0.5 - _topPaddingMult);
    float barWidth = (_width * (1.0 - _leftPaddingMult)) / ((float)_numBars);

    float halfWidth = barWidth * 0.75 * 0.5;
    float currentPos = graphLeft + (barWidth / 2.0);

    osg::Vec3Array * verts = dynamic_cast<osg::Vec3Array*>(_barGeom->getVertexArray());
    if(!verts)
    {
	std::cerr << "Invalid vertex array." << std::endl;
	return;
    }

    int vertIndex = 0;

    for(int i = 0; i < _groupOrder.size(); ++i)
    {
	if(_data.find(_groupOrder[i]) == _data.end())
	{
	    continue;
	}
	for(int j = 0; j < _data[_groupOrder[i]].size(); ++j)
	{
	    float value = _data[_groupOrder[i]][j].second;
	    if(value > _maxDisplayRange)
	    {
		value = _maxDisplayRange;
	    }
	    if(value < _minDisplayRange)
	    {
		verts->at(vertIndex) = osg::Vec3(0,0,0);
		verts->at(vertIndex+1) = osg::Vec3(0,0,0);
		verts->at(vertIndex+2) = osg::Vec3(0,0,0);
		verts->at(vertIndex+3) = osg::Vec3(0,0,0);
	    }
	    else
	    {
		switch(_axisType)
		{
		    case BGAT_LINEAR:
		    {
			break;
		    }
		    case BGAT_LOG:
		    {
			value = log10(value);
			float logMin = log10(_minDisplayRange);
			float logMax = log10(_maxDisplayRange);
			float barHeight = ((value - logMin) / (logMax - logMin)) * (graphTop - graphBottom);
			barHeight += graphBottom;

			verts->at(vertIndex) = osg::Vec3(currentPos+halfWidth,-1,barHeight);
			verts->at(vertIndex+1) = osg::Vec3(currentPos+halfWidth,-1,graphBottom);
			verts->at(vertIndex+2) = osg::Vec3(currentPos-halfWidth,-1,graphBottom);
			verts->at(vertIndex+3) = osg::Vec3(currentPos-halfWidth,-1,barHeight);

			break;
		    }
		    default:
			break;
		}
	    }

	    vertIndex += 4;
	    currentPos += barWidth;
	}
    }
}

void GroupedBarGraph::updateAxis()
{
    _axisGeode->removeDrawables(0,_axisGeode->getNumDrawables());

    //find value of bottom padding mult
    std::vector<osgText::Text*> textList;

    osg::Quat q;
    q.makeRotate(M_PI/2.0,osg::Vec3(1.0,0,0));
    q = q * osg::Quat(-M_PI/2.0,osg::Vec3(0,1.0,0));

    for(int i = 0; i < _groupOrder.size(); ++i)
    {
	if(_data.find(_groupOrder[i]) == _data.end())
	{
	    continue;
	}
	for(int j = 0; j < _data[_groupOrder[i]].size(); ++j)
	{
	    osgText::Text * text = makeText(_data[_groupOrder[i]][j].first,osg::Vec4(0,0,0,1));
	    text->setRotation(q);
	    text->setAlignment(osgText::Text::RIGHT_CENTER);
	    textList.push_back(text);
	}
    }

    float minWidthValue = FLT_MAX;
    float maxWidthValue = FLT_MIN;

    float minHeightValue = FLT_MAX;
    float maxHeightValue = FLT_MIN;

    for(int i = 0; i < textList.size(); ++i)
    {
	osg::BoundingBox bb;
	bb = textList[i]->getBound();

	float width = bb.xMax() - bb.xMin();
	float height = bb.zMax() - bb.zMin();

	//std::cerr << "Width: " << width << " Height: " << height << std::endl;

	if(width < minWidthValue)
	{
	    minWidthValue = width;
	}
	if(width > maxWidthValue)
	{
	    maxWidthValue = width;
	}
	if(height < minHeightValue)
	{
	    minHeightValue = height;
	}
	if(height > maxHeightValue)
	{
	    maxHeightValue = height;
	}
    }

    float tickSize = _height * 0.01;

    //std::cerr << "Max width: " << maxWidthValue << " height: " << maxHeightValue << std::endl;
    float targetWidth = ((_width * (1.0 - _leftPaddingMult)) / ((float)_numBars)) * 0.9;
    float charSize = targetWidth / maxWidthValue;
    float maxSize = _maxBottomPaddingMult * _height - 2.0 * tickSize;

    if(charSize * maxHeightValue > maxSize)
    {
	charSize *= maxSize / (charSize * maxHeightValue);
	_currentBottomPaddingMult = _maxBottomPaddingMult;
    }
    else
    {
	_currentBottomPaddingMult = (maxHeightValue + 2.0 * tickSize) / _height;
    }

    float barWidth = (_width * (1.0 - _leftPaddingMult)) / ((float)_numBars);
    float currentX = (_width * _leftPaddingMult) - (_width / 2.0);
    float zValue = (_height * _currentBottomPaddingMult) - (_height / 2.0);

    currentX += barWidth / 2.0;

    //std::cerr << "CharSize: " << charSize << std::endl;

    for(int i = 0; i < textList.size(); ++i)
    {
	//std::cerr << "text currentx: " << currentX << " z: " << zValue << std::endl;
	textList[i]->setPosition(osg::Vec3(currentX,-1,zValue));
	textList[i]->setCharacterSize(charSize);
	_axisGeode->addDrawable(textList[i]);
	currentX += barWidth;
    }

    // lets make some lines
    osg::Geometry * lineGeom = new osg::Geometry();
    _axisGeode->addDrawable(lineGeom);

    osg::Vec3Array * lineVerts = new osg::Vec3Array();
    osg::Vec4Array * lineColors = new osg::Vec4Array(1);
    lineColors->at(0) = osg::Vec4(0,0,0,1);

    lineGeom->setVertexArray(lineVerts);
    lineGeom->setColorArray(lineColors);
    lineGeom->setColorBinding(osg::Geometry::BIND_OVERALL);

    float graphLeft = _width * (_leftPaddingMult - 0.5);
    float graphBottom = _height * (_currentBottomPaddingMult - 0.5);
    float graphTop = _height * (0.5 - _topPaddingMult);

    // graph top
    lineVerts->push_back(osg::Vec3(graphLeft,-1,graphTop));
    lineVerts->push_back(osg::Vec3(_width/2.0,-1,graphTop));

    // graph bottom
    lineVerts->push_back(osg::Vec3(graphLeft,-1,graphBottom));
    lineVerts->push_back(osg::Vec3(_width/2.0,-1,graphBottom));

    float titleMult = 0.4;
    float topLabelMult = 0.25;
    float groupLabelMult = 0.35;

    float barTop = graphTop + groupLabelMult * _topPaddingMult * _height;
    float barPos = graphLeft;

    lineVerts->push_back(osg::Vec3(barPos,-1,barTop));
    lineVerts->push_back(osg::Vec3(barPos,-1,-_height/2.0));

    for(int i = 0; i < _groupOrder.size(); i++)
    {
	if(_data.find(_groupOrder[i]) == _data.end())
	{
	    continue;
	}

	barPos += barWidth * _data[_groupOrder[i]].size();
	lineVerts->push_back(osg::Vec3(barPos,-1,barTop));
	lineVerts->push_back(osg::Vec3(barPos,-1,-_height/2.0));
    }

    switch(_axisType)
    {
	case BGAT_LINEAR:
	{
	    std::cerr << "Linear graph label not yet implemented." << std::endl;
	    break;
	}
	case BGAT_LOG:
	{
	    float tickHeight = graphTop;
	    float currentTickValue = _maxDisplayRange;
	    float interval = (1.0 / (log10(_maxDisplayRange) - log10(_minDisplayRange))) * (graphTop - graphBottom);

	    float tickCharacterSize;
	    int maxExp = (int)std::max(fabs(log10(_maxDisplayRange)),fabs(log10(_minDisplayRange)));
	    maxExp += 2;

	    std::stringstream testss;
	    while(maxExp > 0)
	    {
		testss << "0";
		maxExp--;
	    }

	    osg::ref_ptr<osgText::Text> testText = makeText(testss.str(),osg::Vec4(0,0,0,1));
	    osg::BoundingBox testbb = testText->getBound();
	    float testWidth = testbb.xMax() - testbb.xMin();

	    tickCharacterSize = (_leftPaddingMult * _width * 0.6 - 2.0 * tickSize) / testWidth;

	    while(tickHeight >= graphBottom)
	    {
		lineVerts->push_back(osg::Vec3(graphLeft-tickSize,-1,tickHeight));
		lineVerts->push_back(osg::Vec3(graphLeft,-1,tickHeight));

		std::stringstream tss;
		tss << currentTickValue;
		osgText::Text * tickText = makeText(tss.str(),osg::Vec4(0,0,0,1));
		tickText->setAlignment(osgText::Text::RIGHT_CENTER);
		tickText->setCharacterSize(tickCharacterSize);
		tickText->setPosition(osg::Vec3(graphLeft - 2.0*tickSize,-1,tickHeight));
		_axisGeode->addDrawable(tickText);

		currentTickValue /= 10.0;
		tickHeight -= interval;
	    }

	    tickHeight = graphTop;
	    currentTickValue = _maxDisplayRange;

	    int count = -1;
	    float tickReduc = _maxDisplayRange / 10.0;
	    while(tickHeight >= graphBottom)
	    { 
		count++;
		lineVerts->push_back(osg::Vec3(graphLeft - 0.5*tickSize,-1,tickHeight));
		lineVerts->push_back(osg::Vec3(graphLeft,-1,tickHeight));

		//std::cerr << "CurrentTickValue: " << currentTickValue << " height: " << tickHeight << std::endl;

		if((count % 10) == 9)
		{
		    tickReduc /= 10.0;
		    count++;
		}
		currentTickValue -= tickReduc;
		tickHeight = ((log10(currentTickValue) - log10(_minDisplayRange)) / (log10(_maxDisplayRange) - log10(_minDisplayRange))) * (graphTop - graphBottom);
		tickHeight += graphBottom;

		//std::cerr << "Post count: " << count << " tickRed: " << tickReduc << " CurrentTickValue: " << currentTickValue << " height: " << tickHeight << std::endl;
	    }

	    break;
	}
	default:
	{
	    std::cerr << "Unknown axis type." << std::endl;
	    break;
	}
    }

    // value axis label
    std::stringstream valueAxisss;
    valueAxisss << _axisLabel << " (" << _axisUnits << ")";

    osgText::Text * vaText = makeText(valueAxisss.str(),osg::Vec4(0,0,0,1));
    vaText->setRotation(q);
    osg::BoundingBox bb = vaText->getBound();
    float csize = (0.4 * 0.85 * _leftPaddingMult * _width) / (bb.xMax() - bb.xMin());
    vaText->setCharacterSize(csize);
    vaText->setAlignment(osgText::Text::CENTER_CENTER);
    vaText->setPosition(osg::Vec3(-(_width / 2.0) + (_width * _leftPaddingMult * 0.2),-1,(graphTop - graphBottom) / 2.0 + graphBottom));
    _axisGeode->addDrawable(vaText);

    // graphTitle
    osgText::Text * titleText = makeText(_title,osg::Vec4(0,0,0,1));
    titleText->setAlignment(osgText::Text::CENTER_CENTER);
    bb = titleText->getBound();
    csize = (titleMult * 0.85 * _topPaddingMult * _height) / (bb.zMax() - bb.zMin());
    titleText->setCharacterSize(csize);
    titleText->setPosition(osg::Vec3(0,-1,(_height / 2.0) - (titleMult * _topPaddingMult * _height * 0.5)));
    _axisGeode->addDrawable(titleText);

    // group label
    titleText = makeText(_groupLabel,osg::Vec4(0,0,0,1));
    titleText->setAlignment(osgText::Text::CENTER_CENTER);
    bb = titleText->getBound();
    csize = (topLabelMult * 0.85 * _topPaddingMult * _height) / (bb.zMax() - bb.zMin());
    titleText->setCharacterSize(csize);
    titleText->setPosition(osg::Vec3(graphLeft + ((_width / 2.0) - graphLeft) / 2.0,-1,(_height / 2.0) - ((titleMult+(topLabelMult/2.0)) * _topPaddingMult * _height)));
    _axisGeode->addDrawable(titleText);

    osg::ref_ptr<osgText::Text> tempText = makeText("Ay",osg::Vec4(0,0,0,1));
    bb = tempText->getBound();
    csize = (groupLabelMult * 0.7 * _topPaddingMult * _height) / (bb.zMax() - bb.zMin());

    // group labels
    float groupStart = graphLeft;
    for(int i = 0; i < _groupOrder.size(); i++)
    {
	if(_data.find(_groupOrder[i]) == _data.end())
	{
	    continue;
	}
	osgText::Text * text = makeText(_groupOrder[i],osg::Vec4(0,0,0,1));
	text->setAlignment(osgText::Text::CENTER_CENTER);
	//bb = text->getBound();
	//csize = (groupLabelMult * 0.7 * _topPaddingMult * _height) / (bb.zMax() - bb.zMin());
	text->setCharacterSize(csize);

	float halfWidth = _data[_groupOrder[i]].size() * barWidth * 0.5;
	text->setPosition(osg::Vec3(groupStart + halfWidth,-1,graphTop + (groupLabelMult * 0.5 * _topPaddingMult * _height)));
	makeTextFit(text,2.0*halfWidth * 0.95);
	_axisGeode->addDrawable(text);

	groupStart += 2.0 * halfWidth;
    }

    lineGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES,0,lineVerts->size()));
}

osgText::Text * GroupedBarGraph::makeText(std::string text, osg::Vec4 color)
{
    osgText::Text * textNode = new osgText::Text();
    textNode->setCharacterSize(1.0);
    textNode->setAlignment(osgText::Text::CENTER_CENTER);
    textNode->setColor(color);
    textNode->setBackdropColor(osg::Vec4(0,0,0,0));
    textNode->setAxisAlignment(osgText::Text::XZ_PLANE);
    textNode->setText(text);
    if(_font)
    {
	textNode->setFont(_font);
    }
    return textNode;
}

void GroupedBarGraph::makeTextFit(osgText::Text * text, float maxSize)
{
    osg::BoundingBox bb = text->getBound();
    float width = bb.xMax() - bb.xMin();
    if(width <= maxSize)
    {
	return;
    }

    std::string str = text->getText().createUTF8EncodedString();
    if(!str.length())
    {
	return;
    }

    while(str.length() > 1)
    {
	str = str.substr(0,str.length()-1);
	text->setText(str + "..");
	bb = text->getBound();
	width = bb.xMax() - bb.xMin();
	if(width <= maxSize)
	{
	    return;
	}
    }

    str += ".";
    text->setText(str);
}
