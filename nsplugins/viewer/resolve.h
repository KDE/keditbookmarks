/*
  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 
*/                                                                            

#ifndef VIEWER_RESOLVE_H
#define VIEWER_RESOLVE_H

#define RESOLVE_RETVAL(fname,error)           \
  kDebug() << "NSPluginInstance::";  \
                                              \
  if (!_handle)                               \
    return error;                             \
                                              \
  if (!func_ ## fname)                        \
    func_ ## fname = _handle->symbol("NPP_"#fname); \
                                              \
  if (!func_ ## fname)                        \
  {                                           \
    kDebug() << "Failed: NPP_";      \
    return error;                             \
  }                                           \
  kDebug() << "Resolved NPP_";


#define RESOLVE(fname) RESOLVE_RETVAL(fname, NPERR_GENERIC_ERROR)
#define RESOLVE_VOID(fname) RESOLVE_RETVAL(fname, ;)


#define CHECK(fname,error)                    \
  kDebug() << "results in " << error;         \
  return error;

#endif /* VIEWER_RESOLVE_H */

