#include "stdafx.h"
#include "ColPlugin.h"

using namespace CallbackSpace;

HitCallback::HitCallback(bool whichTitle)	: selectedNode(NULL)
{
	if (whichTitle)
		this->dlgTitle= "Pick collision mesh";
	else
		this->dlgTitle= "Pick shadow mesh";
}

MCHAR* HitCallback::dialogTitle()
{
	return const_cast<char*>(this->dlgTitle.c_str());
}

MCHAR* ::HitCallback::buttonText()
{
	return "Confirm";
}

BOOL HitCallback::singleSelect()
{
	return TRUE;
}

BOOL HitCallback::useFilter()
{
	return TRUE;
}

BOOL HitCallback::filter(INode* laNode)
{
	::ObjectState os= laNode->EvalWorldState(0);
	if (os.obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID,0)))
		return TRUE;
	else
		return FALSE;
}


BOOL HitCallback::useProc()
{
	return TRUE;
}

void HitCallback::proc(INodeTab& nodeTab)
{
	if (nodeTab.Count())
		this->selectedNode= nodeTab[0];
}

BOOL HitCallback::doCustomHilite()
{
	return TRUE;
}

BOOL HitCallback::doHilite(INode* node)
{
	return FALSE;
}

BOOL HitCallback::showHiddenAndFrozen()
{
	return TRUE;
}