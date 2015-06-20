#ifndef __FSELECT_HPP_
#define __FSELECT_HPP_

#include "jwindow.hpp"



jwindow *file_dialog(window_manager *wm, char *prompt, char *def,
		     int ok_id, char *ok_name, int cancel_id, char *cancel_name,
		     char *FILENAME_str,
		     int filename_id);

#endif




