/*
 * Listview class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_LISTVIEW_H
#define __WINE_LISTVIEW_H

typedef struct tagLISTVIEW_SUBITEM
{
    LPSTR pszText;
    INT iImage;
    INT iSubItem;

} LISTVIEW_SUBITEM;

typedef struct tagLISTVIEW_ITEM
{
    UINT state;
    LPSTR  pszText;
    INT  iImage;
    LPARAM lParam;
    INT  iIndent;

} LISTVIEW_ITEM;


typedef struct tagLISTVIEW_INFO
{
    COLORREF   clrBk;
    COLORREF   clrText;
    COLORREF   clrTextBk;
    HIMAGELIST himlNormal;
    HIMAGELIST himlSmall;
    HIMAGELIST himlState;
    BOOL bLButtonDown;
    BOOL bRButtonDown;
    INT nCountPerColumn;
    INT nCountPerRow;
    INT nFocusedItem;
    INT nItemHeight;
    INT nColumnWidth;
    INT nSelectionMark;
    SHORT notifyFormat;
    RECT rcList;
    SIZE iconSize;
    SIZE smallIconSpacing;
    SIZE largeIconSpacing;
    UINT uCallbackMask;
    HWND     hwndHeader;
    HFONT    hDefaultFont;
    HFONT    hFont;
    BOOL     bFocus;
    DWORD      dwExStyle;    /* extended listview style */
    HDPA       hdpaItems;

} LISTVIEW_INFO;


extern VOID LISTVIEW_Register (VOID);
extern VOID LISTVIEW_Unregister (VOID);

#endif  /* __WINE_LISTVIEW_H */
