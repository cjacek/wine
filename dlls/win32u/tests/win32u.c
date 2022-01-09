/*
 * Copyright 2021 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include "wine/test.h"

#include "winbase.h"
#include "ntuser.h"


static void test_NtUserEnumDisplayDevices(void)
{
    NTSTATUS ret;
    DISPLAY_DEVICEW info = { sizeof(DISPLAY_DEVICEW) };

    SetLastError( 0xdeadbeef );
    ret = NtUserEnumDisplayDevices( NULL, 0, &info, 0 );
    ok( !ret && GetLastError() == 0xdeadbeef,
        "NtUserEnumDisplayDevices returned %x %u\n", ret, GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = NtUserEnumDisplayDevices( NULL, 12345, &info, 0 );
    ok( ret == STATUS_UNSUCCESSFUL && GetLastError() == 0xdeadbeef,
                  "NtUserEnumDisplayDevices returned %x %u\n", ret,
                  GetLastError() );

    info.cb = 0;

    SetLastError( 0xdeadbeef );
    ret = NtUserEnumDisplayDevices( NULL, 0, &info, 0 );
    ok( ret == STATUS_UNSUCCESSFUL && GetLastError() == 0xdeadbeef,
        "NtUserEnumDisplayDevices returned %x %u\n", ret, GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = NtUserEnumDisplayDevices( NULL, 12345, &info, 0 );
    ok( ret == STATUS_UNSUCCESSFUL && GetLastError() == 0xdeadbeef,
        "NtUserEnumDisplayDevices returned %x %u\n", ret, GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = NtUserEnumDisplayDevices( NULL, 0, NULL, 0 );
    ok( ret == STATUS_UNSUCCESSFUL && GetLastError() == 0xdeadbeef,
        "NtUserEnumDisplayDevices returned %x %u\n", ret, GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = NtUserEnumDisplayDevices( NULL, 12345, NULL, 0 );
    ok( ret == STATUS_UNSUCCESSFUL && GetLastError() == 0xdeadbeef,
        "NtUserEnumDisplayDevices returned %x %u\n", ret, GetLastError() );
}

static void test_NtUserCloseWindowStation(void)
{
    BOOL ret;

    SetLastError( 0xdeadbeef );
    ret = NtUserCloseWindowStation( 0 );
    ok( !ret && GetLastError() == ERROR_INVALID_HANDLE,
        "NtUserCloseWindowStation returned %x %u\n", ret, GetLastError() );
}

static void test_window_props(void)
{
    HANDLE prop;
    ATOM atom;
    HWND hwnd;
    BOOL ret;

    hwnd = CreateWindowExA( 0, "static", NULL, WS_POPUP, 0,0,0,0,0,0,0, NULL );

    atom = GlobalAddAtomW( L"test" );

    ret = NtUserSetProp( hwnd, UlongToPtr(atom), UlongToHandle(0xdeadbeef) );
    ok( ret, "NtUserSetProp failed: %u\n", GetLastError() );

    prop = GetPropW( hwnd, L"test" );
    ok( prop == UlongToHandle(0xdeadbeef), "prop = %p\n", prop );

    prop = NtUserGetProp( hwnd, UlongToPtr(atom) );
    ok( prop == UlongToHandle(0xdeadbeef), "prop = %p\n", prop );

    prop = NtUserRemoveProp( hwnd, UlongToPtr(atom) );
    ok( prop == UlongToHandle(0xdeadbeef), "prop = %p\n", prop );

    prop = GetPropW(hwnd, L"test");
    ok(!prop, "prop = %p\n", prop);

    GlobalDeleteAtom( atom );
    DestroyWindow( hwnd );
}

static BOOL WINAPI count_win( HWND hwnd, LPARAM lparam )
{
    ULONG *cnt = (ULONG *)lparam;
    (*cnt)++;
    return TRUE;
}

static void test_NtUserBuildHwndList(void)
{
    ULONG size, desktop_windows_cnt;
    HWND buf[512], hwnd;
    NTSTATUS status;

    size = 0;
    status = NtUserBuildHwndList( 0, 0, 0, 0, GetCurrentThreadId(), ARRAYSIZE(buf), buf, &size );
    ok( !status, "NtUserBuildHwndList failed: %#x\n", status );
    ok( size == 1, "size = %u\n", size );
    ok( buf[0] == HWND_BOTTOM, "buf[0] = %p\n", buf[0] );

    hwnd = CreateWindowExA( 0, "static", NULL, WS_POPUP, 0,0,0,0,GetDesktopWindow(),0,0, NULL );

    size = 0;
    status = NtUserBuildHwndList( 0, 0, 0, 0, GetCurrentThreadId(), ARRAYSIZE(buf), buf, &size );
    ok( !status, "NtUserBuildHwndList failed: %#x\n", status );
    ok( size == 3, "size = %u\n", size );
    ok( buf[0] == hwnd, "buf[0] = %p\n", buf[0] );
    ok( buf[2] == HWND_BOTTOM, "buf[0] = %p\n", buf[2] );

    size = 0;
    status = NtUserBuildHwndList( 0, 0, 0, 0, GetCurrentThreadId(), 3, buf, &size );
    ok( !status, "NtUserBuildHwndList failed: %#x\n", status );
    ok( size == 3, "size = %u\n", size );

    size = 0;
    status = NtUserBuildHwndList( 0, 0, 0, 0, GetCurrentThreadId(), 2, buf, &size );
    ok( status == STATUS_BUFFER_TOO_SMALL, "NtUserBuildHwndList failed: %#x\n", status );
    ok( size == 3, "size = %u\n", size );

    size = 0;
    status = NtUserBuildHwndList( 0, 0, 0, 0, GetCurrentThreadId(), 1, buf, &size );
    ok( status == STATUS_BUFFER_TOO_SMALL, "NtUserBuildHwndList failed: %#x\n", status );
    ok( size == 3, "size = %u\n", size );

    desktop_windows_cnt = 0;
    EnumDesktopWindows( 0, count_win, (LPARAM)&desktop_windows_cnt );

    size = 0;
    status = NtUserBuildHwndList( 0, 0, 0, 1, 0, ARRAYSIZE(buf), buf, &size );
    ok( !status, "NtUserBuildHwndList failed: %#x\n", status );
    ok( size == desktop_windows_cnt + 1, "size = %u, expected %u\n", size, desktop_windows_cnt + 1 );

    desktop_windows_cnt = 0;
    EnumDesktopWindows( GetThreadDesktop( GetCurrentThreadId() ), count_win, (LPARAM)&desktop_windows_cnt );

    size = 0;
    status = NtUserBuildHwndList( GetThreadDesktop(GetCurrentThreadId()), 0, 0, 1, 0,
                                  ARRAYSIZE(buf), buf, &size );
    ok( !status, "NtUserBuildHwndList failed: %#x\n", status );
    ok( size == desktop_windows_cnt + 1, "size = %u, expected %u\n", size, desktop_windows_cnt + 1 );

    size = 0;
    status = NtUserBuildHwndList( GetThreadDesktop(GetCurrentThreadId()), 0, 0, 0, 0,
                                  ARRAYSIZE(buf), buf, &size );
    ok( !status, "NtUserBuildHwndList failed: %#x\n", status );
    todo_wine
    ok( size > desktop_windows_cnt + 1, "size = %u, expected %u\n", size, desktop_windows_cnt + 1 );

    size = 0xdeadbeef;
    status = NtUserBuildHwndList( UlongToHandle(0xdeadbeef), 0, 0, 0, 0,
                                  ARRAYSIZE(buf), buf, &size );
    ok( status == STATUS_INVALID_HANDLE, "NtUserBuildHwndList failed: %#x\n", status );
    ok( size == 0xdeadbeef, "size = %u\n", size );

    DestroyWindow( hwnd );
}

static void CALLBACK win_event_proc(HWINEVENTHOOK hevent, DWORD event, HWND hwnd, LONG object_id,
                                    LONG child_id, DWORD thread_id, DWORD event_time)
{
}

static const USER_HANDLE_ENTRY *get_handle_entry( USER_SHARED_INFO *shared_info, HANDLE handle,
                                                  unsigned int type )
{
    struct user_session_info *session_info = (void *)shared_info->session_info;
    unsigned int index = LOWORD(handle);
    const USER_HANDLE_ENTRY *entry;

    ok( index <= session_info->nb_handles, "index > nb_handles\n" );
    entry = (const USER_HANDLE_ENTRY *)shared_info->handles + index;
    ok( entry->type == type, "bType = %x, expected %x\n", entry->type, type );
    ok( entry->generation == HIWORD(handle), "bUniq = %x, expectex %x\n", entry->generation, HIWORD(handle) );
    todo_wine
    ok( entry->tid == GetCurrentThreadId(), "pid = %x\n", entry->pid );
    return entry;
}

static void test_shared_info(void)
{
    MEMORY_BASIC_INFORMATION info;
    HWINEVENTHOOK hook;
    USER_SHARED_INFO *shared_info;
    const USER_HANDLE_ENTRY *entry;
    SIZE_T size;
    HWND hwnd;

    shared_info = (void *)GetProcAddress( GetModuleHandleW( L"user32.dll" ), "gSharedInfo" );
    ok(!!shared_info, "shared_info not found\n");

    size = VirtualQuery( (void *)shared_info->session_info, &info, sizeof(info) );
    ok( size == sizeof(info), "VirtualQuery returned %lu\n", size );
    ok( info.AllocationProtect == PAGE_READONLY, "AllocationProtect = %x\n", info.AllocationProtect );

    size = VirtualQuery( (void *)shared_info->handles, &info, sizeof(info) );
    ok( size == sizeof(info), "VirtualQuery returned %Iu\n", size );
    ok( info.AllocationProtect == PAGE_READONLY, "AllocationProtect = %x\n", info.AllocationProtect );

    hwnd = CreateWindowExA( 0, "static", NULL, WS_POPUP, 0,0,0,0,0,0,0, NULL );

    entry = get_handle_entry( shared_info, hwnd, NTUSER_OBJ_WINDOW );
    DestroyWindow( hwnd );
    ok( !entry->type, "type = %x\n", entry->type );

    hook = SetWinEventHook( 0, 0, 0, win_event_proc, GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT );
    entry = get_handle_entry( shared_info, hook, NTUSER_OBJ_HOOK );
    UnhookWinEvent( hook );
    ok( !entry->type, "type = %x\n", entry->type );

    /* the first entry is never used as a valid handle */
    entry = (const USER_HANDLE_ENTRY *)shared_info->handles;
    ok(!entry->type, "type = %x\n", entry->type);
}

START_TEST(win32u)
{
    /* native win32u.dll fails if user32 is not loaded, so make sure it's fully initialized */
    GetDesktopWindow();

    test_NtUserEnumDisplayDevices();
    test_window_props();
    test_NtUserBuildHwndList();
    test_shared_info();
    test_NtUserCloseWindowStation();
}
