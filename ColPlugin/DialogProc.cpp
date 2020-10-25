//Dialog procedure

#include "stdafx.h"
#include "ColPlugin.h"
#include "extras.h"
//progress function

DWORD WINAPI ProgressFunction(LPVOID arg)
{
	return 0;
}
 
INT_PTR CALLBACK ToolDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	ColPlugin* localPlugin= (ColPlugin*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch(Message)
	{
		case WM_INITDIALOG:
		{
			localPlugin = (ColPlugin*)lParam;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)localPlugin);
			SetFocus(hwnd);		//important otherwise messages can't be sent properly. Application will crash after next lines of code.

			//::ICustButton* leBouton= ::GetICustButton(GetDlgItem(localPlugin->hRollup, IDC_CUSTOM1));		//do not use hRollup since it hasn't been initialized yet.
			//leBouton->SetType(CBT_CHECK);
			//leBouton->SetTooltip(true, "tooltip");
			//leBouton->SetText("Click me");
			//leBouton->SetHighlightColor(RED_WASH);
			//ReleaseICustButton(leBouton);

			//set dialog box info
			using namespace extras;
			std::string INIFileName= ::IPathConfigMgr::GetPathConfigMgr()->GetDir(APP_PLUGCFG_DIR);
			if ( *INIFileName.rbegin() != '\\')
				INIFileName += '\\';

			INIFileName += "GTAColPlugin.ini";
			union {
				UINT BoundCBoxState, GTAMatCBoxState, IgnoreShadowCBoxState;
			};

			BoundCBoxState= ::GetPrivateProfileInt(GetRscString(IDS_MAIN).c_str(), GetRscString(IDS_SHOW_BOUNDING).c_str(), 0, INIFileName.c_str());
			if (BoundCBoxState)
				SendMessage(GetDlgItem(hwnd, IDC_BOUNDING), BM_SETCHECK, BST_CHECKED, NULL);
			else
				SendMessage(GetDlgItem(hwnd, IDC_BOUNDING), BM_SETCHECK, BST_UNCHECKED, NULL);

			GTAMatCBoxState= ::GetPrivateProfileInt(GetRscString(IDS_MAIN).c_str(), GetRscString(IDS_USE_GTAMAT).c_str(), 0, INIFileName.c_str());
			if (GTAMatCBoxState)
				SendMessage(GetDlgItem(hwnd, IDC_OPTGTAMAT), BM_SETCHECK, BST_CHECKED, NULL);
			else
				SendMessage(GetDlgItem(hwnd, IDC_OPTGTAMAT), BM_SETCHECK, BST_UNCHECKED, NULL);

			IgnoreShadowCBoxState= ::GetPrivateProfileInt(GetRscString(IDS_MAIN).c_str(), GetRscString(IDS_IGNORE_SHADOW).c_str(), 0, INIFileName.c_str());
			if (IgnoreShadowCBoxState)
				SendMessage(GetDlgItem(hwnd, IDC_IGNORE_SHADOW), BM_SETCHECK, BST_CHECKED, NULL);
			else
				SendMessage(GetDlgItem(hwnd, IDC_IGNORE_SHADOW), BM_SETCHECK, BST_UNCHECKED, NULL);
		}
		break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					localPlugin->iColStream.close();
					//delete existing strings
					LRESULT lastItemIdx= SendMessage(GetDlgItem(localPlugin->hRollup, IDC_COLLIST), LB_GETCOUNT, NULL, NULL);
					while ((lastItemIdx= SendMessage(GetDlgItem(localPlugin->hRollup, IDC_COLLIST), LB_DELETESTRING, (WPARAM)--lastItemIdx, NULL)) != LB_ERR);
					//open file dialog
					::OPENFILENAME ofn;
					strcpy(localPlugin->szFileName,"");
					ZeroMemory(&ofn, sizeof(ofn));

					ofn.lStructSize= sizeof(ofn);
					ofn.hwndOwner= hwnd;
					ofn.lpstrFilter= "Collision Files (*.col)\0*.col\0Model Files (*.dff)\0*.dff\0";
					ofn.lpstrFile= localPlugin->szFileName;
					ofn.nMaxFile= MAX_PATH;
					ofn.Flags= OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
					ofn.lpstrDefExt = "col";

					//The WM_COMMAND message stores the control's window handle in the LPARAM parameter
					if (GetOpenFileName(&ofn))
						SendMessage((HWND)lParam, WM_SETTEXT, NULL, (LPARAM)extras::GetShortFilename(localPlugin->szFileName).c_str());

					else
					{
						SendMessage((HWND)lParam, WM_SETTEXT, NULL, (LPARAM)"Open Collision File");
						break;
					}
					localPlugin->iColStream.open(ofn.lpstrFile, std::ios::in | std::ios::beg | std::ios::binary);
					std::string fileExt= (ofn.lpstrFile+ofn.nFileExtension);
					std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), std::ptr_fun(toupper));
					//check if DFF
					if ((fileExt == "DFF") && ! (localPlugin->SeekDFFCollision()))
						break;
					localPlugin->PopulateListbox();
				}
				break;

				case IDC_LOADCOL:
				{
					LRESULT selCount= SendMessage(GetDlgItem(hwnd, IDC_COLLIST), LB_GETSELCOUNT, NULL, NULL);
					localPlugin->LboxSelection= new int[selCount];
					SendMessage(GetDlgItem(hwnd, IDC_COLLIST), LB_GETSELITEMS, (WPARAM)selCount, (LPARAM)localPlugin->LboxSelection);
					//disable redraw
					localPlugin->ColIP->DisableSceneRedraw();
					localPlugin->ColIP->ProgressStart("Importing", TRUE, ProgressFunction, NULL);
					for (int i= 0; i < selCount; ++i)
					{
						localPlugin->PercentageProgress= (100.0f * i)/selCount;
						localPlugin->ImportCollisions(localPlugin->LboxSelection[i]);
					}
					delete[] localPlugin->LboxSelection;
					localPlugin->ColIP->ProgressEnd();
					//enable redraw
					localPlugin->ColIP->EnableSceneRedraw();
					localPlugin->ColIP->ForceCompleteRedraw();
				}
				break;

				case IDC_DELETE:
					localPlugin->DeleteCollisions();
				break;

				case IDCANCEL:
				{
					localPlugin->iColStream.close();
					using namespace extras;
					std::string INIFileName= ::IPathConfigMgr::GetPathConfigMgr()->GetDir(APP_PLUGCFG_DIR);
					if ( *INIFileName.rbegin() != '\\')
						INIFileName += '\\';

					INIFileName += "GTAColPlugin.ini";

					union {
						UINT BoundCBoxState, GTAMatCBoxState, IgnoreShadowCBoxState;
					};
					
					BoundCBoxState= SendMessage(GetDlgItem(hwnd, IDC_BOUNDING), BM_GETCHECK, NULL, NULL);

					if (BoundCBoxState== BST_CHECKED)
						WritePrivateProfileString(GetRscString(IDS_MAIN).c_str(), GetRscString(IDS_SHOW_BOUNDING).c_str(), "1", INIFileName.c_str());
					else
						WritePrivateProfileString(GetRscString(IDS_MAIN).c_str(), GetRscString(IDS_SHOW_BOUNDING).c_str(), "0", INIFileName.c_str());

					GTAMatCBoxState= SendMessage(GetDlgItem(hwnd, IDC_OPTGTAMAT), BM_GETCHECK, NULL, NULL);
					if (GTAMatCBoxState== BST_CHECKED)
						WritePrivateProfileString(GetRscString(IDS_MAIN).c_str(), GetRscString(IDS_USE_GTAMAT).c_str(), "1", INIFileName.c_str());
					else
						WritePrivateProfileString(GetRscString(IDS_MAIN).c_str(), GetRscString(IDS_USE_GTAMAT).c_str(), "0", INIFileName.c_str());

					IgnoreShadowCBoxState= SendMessage(GetDlgItem(hwnd, IDC_IGNORE_SHADOW), BM_GETCHECK, NULL, NULL);
					if (IgnoreShadowCBoxState== BST_CHECKED)
						WritePrivateProfileString(GetRscString(IDS_MAIN).c_str(), GetRscString(IDS_IGNORE_SHADOW).c_str(), "1", INIFileName.c_str());
					else
						WritePrivateProfileString(GetRscString(IDS_MAIN).c_str(), GetRscString(IDS_IGNORE_SHADOW).c_str(), "0", INIFileName.c_str());

					localPlugin->ColIU->CloseUtility();
				}
				break;

				case IDC_CUSTOM1:
				{
	
				}
				break;
			}
		break;

		default:
			return FALSE;
		break;
	}
	return TRUE;
}