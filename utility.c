/*******************************************************************************
*   ___   workplace
*  ¦OUX¦  ‟gtk+” condensed and ‘unix’
*  ¦Inc¦  plugin
*   ---   ‟geany” adaptation
*         source code, “make”, “.cx”, system administration and unused tools
* ©overcq                on ‟Gentoo Linux 13.0” “x86_64”             2015‒1‒13 *
*******************************************************************************/
// Założenia:
// “stdout” i “stderr” “exec()” są w UTF-8.
// Jak również wszystko i tak powinno być w UTF-8, gdzie nie zauważone i dlatego nie wyłączane z obsługi.
//==============================================================================
#include "0.h"
//==============================================================================
static const char *H_ocq_I_open_directory_S_list_file = ".autoopen-text";
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static GtkWidget *H_ocq_E_geany_Q_action_S_toolbar_button = null;
static struct GeanyKeyGroup *H_ocq_E_geany_Q_action_Z_keyboard_group_S;
static GPtrArray *H_ocq_E_geany_Q_action_S_build_target;
static GMutex H_ocq_E_geany_Q_action_S_mutex;
static gsize H_ocq_E_geany_Q_action_Z_menu_I_add_S_action_id;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static GPid H_ocq_E_compile_S_pid = ( GPid )empty;
static GIOChannel *H_ocq_E_compile_S_channel_out, *H_ocq_E_compile_S_channel_err;
static _Bool H_ocq_E_compile_I_make_S_coux_project;
static _Bool H_ocq_E_compile_I_exec_X_watch_S_log_Z_message, H_ocq_E_compile_I_exec_X_watch_S_log_Z_compiler;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static int H_ocq_E_geany_S_last_document = 0, H_ocq_E_geany_S_current_document = 0;
static int H_ocq_E_geany_I_open_directory_X_folder_S_timeout = 0;
static GtkWidget *H_ocq_E_doc_com_S_page = null;
static unsigned H_ocq_E_doc_com_I_idle_update_S = 0;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static GeanyPlugin *H_ocq_E_geany_S_plugin;
#define geany_data H_ocq_E_geany_S_plugin->geany_data
//==============================================================================
static
void
H_ocq_E_geany_I_about_plugin( void
){  dialogs_show_msgbox( GTK_MESSAGE_INFO
    , "this button is placement for toolbar menu of ‟%s” plugin.\n"
      "click arrow on the right to open menu.\n"
      "⁂\n"
      "‟%s %s” by %s\n"
      "%s"
    , H_ocq_E_geany_S_plugin->info->name
    , H_ocq_E_geany_S_plugin->info->name
    , H_ocq_E_geany_S_plugin->info->version
    , H_ocq_E_geany_S_plugin->info->author
    , H_ocq_E_geany_S_plugin->info->description
    );
}
//==============================================================================
static
void
H_ocq_E_geany_I_open_directory_I_hash_free( void *p
){  struct hash_e
    { GPtrArray *globs;
      unsigned priority_1;
    } *hash_e = p;
    g_ptr_array_unref( hash_e->globs );
}
static
int
H_ocq_E_geany_I_open_directory_I_files_slist_cmp(
  void *a_
, void *b_
, void *data
){  struct slist_e
    { char *path;
      unsigned priority_1, priority_2, priority_3;
    } *a = a_, *b = b_;
    int ret = a->priority_1 - b->priority_1;
    if(ret)
        return ret;
    ret = a->priority_2 - b->priority_2;
    if(ret)
        return ret;
    ret = a->priority_3 - b->priority_3;
    if(ret)
        return ret;
    return strcmp( a->path, b->path );
}
static
void
H_ocq_E_geany_I_open_directory_I_slist_free1( void *p
){  struct slist_e
    { char *path;
      unsigned priority_1, priority_2, priority_3;
    } *slist_e = p;
    g_free( slist_e->path );
}
//------------------------------------------------------------------------------
static
gboolean
H_ocq_E_geany_I_open_directory_X_folder_I_timeout( void *chooser
){  char *s = gtk_file_chooser_get_uri(chooser);
    if( !s )
        goto End;
    GFile *dir = g_file_new_for_uri(s);
    g_free(s);
    GFile *file = g_file_get_child( dir, H_ocq_I_open_directory_S_list_file );
    g_object_unref(dir);
    gtk_dialog_set_response_sensitive( chooser, GTK_RESPONSE_ACCEPT, g_file_query_exists( file, null ));
    g_object_unref(file);
End:H_ocq_E_geany_I_open_directory_X_folder_S_timeout = 0;
    return G_SOURCE_REMOVE;
}
static
void
H_ocq_E_geany_I_open_directory_X_select( GtkFileChooser *chooser
, void *data
){  if( H_ocq_E_geany_I_open_directory_X_folder_S_timeout )
        g_source_remove( H_ocq_E_geany_I_open_directory_X_folder_S_timeout );
    H_ocq_E_geany_I_open_directory_X_folder_S_timeout = g_timeout_add( 183, H_ocq_E_geany_I_open_directory_X_folder_I_timeout, chooser );
}
static
void
H_ocq_E_geany_I_open_directory( void
){  GtkWidget *dialog = gtk_file_chooser_dialog_new( "open project directory"
    , ( void * )geany_data->main_widgets->window
    , GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER
    , "_cancel"
    , GTK_RESPONSE_REJECT
    , "_open"
    , GTK_RESPONSE_ACCEPT
    , null
    );
    g_signal_connect(( void * )dialog, "selection-changed", ( void * )H_ocq_E_geany_I_open_directory_X_select, null );
    if( gtk_dialog_run(( void * )dialog ) == GTK_RESPONSE_ACCEPT )
    {   if( H_ocq_E_geany_I_open_directory_X_folder_S_timeout )
        {   g_source_remove( H_ocq_E_geany_I_open_directory_X_folder_S_timeout );
            H_ocq_E_geany_I_open_directory_X_folder_S_timeout = 0;
        }
        GFile *dir = gtk_file_chooser_get_file(( void * )dialog );
        GFile *file = g_file_get_child( dir, H_ocq_I_open_directory_S_list_file );
        GError *error = null;
        GFileInputStream *input_stream;
        if( !( input_stream = g_file_read( file, null, &error )))
        {   g_object_unref(file);
            g_object_unref(dir);
            goto End;
        }
        g_object_unref(file);
        GDataInputStream *data_stream = g_data_input_stream_new(( void * )input_stream );
        g_object_unref( input_stream );
        gsize l;
        char *line;
        if( !( line = g_data_input_stream_read_line_utf8( data_stream, &l, null, &error )))
        {   g_object_unref( data_stream );
            g_object_unref(dir);
            goto End;
        }
        GHashTable *dir_globs = g_hash_table_new_full( g_direct_hash, ( void * )strcmp, g_free, H_ocq_E_geany_I_open_directory_I_hash_free );
        GPtrArray *global_globs, *current_array = g_ptr_array_new_with_free_func(( void * )g_pattern_spec_free );
        unsigned dir_i = 0;
        struct hash_e
        { GPtrArray *globs;
          unsigned priority_1;
        } *hash_e;
        if( *line )
        {   hash_e = g_newa( struct hash_e, 1 );
            hash_e->globs = current_array;
            hash_e->priority_1 = dir_i;
            g_hash_table_insert( dir_globs, line, hash_e );
            global_globs = 0;
        }else
        {   g_free(line);
            global_globs = current_array;
        }
        while( line = g_data_input_stream_read_line_utf8( data_stream, &l, null, &error ))
        {   if( !*line )
            {   g_free(line);
                if( !( line = g_data_input_stream_read_line_utf8( data_stream, &l, null, &error )))
                    break;
                hash_e = g_newa( struct hash_e, 1 );
                hash_e->globs = current_array = g_ptr_array_new_with_free_func(( void * )g_pattern_spec_free );
                hash_e->priority_1 = ++dir_i;
                if( g_hash_table_contains( dir_globs, line ))
                {   g_object_unref( data_stream );
                    if( global_globs )
                        g_ptr_array_unref( global_globs );
                    g_hash_table_unref( dir_globs );
                    g_object_unref(dir);
                    goto End;
                }
                g_hash_table_insert( dir_globs, line, hash_e );
                continue;
            }
            g_ptr_array_add( current_array, g_pattern_spec_new(line) );
            g_free(line);
        }
        g_object_unref( data_stream );
        if(error)
        {   if( global_globs )
                g_ptr_array_unref( global_globs );
            g_hash_table_unref( dir_globs );
            g_object_unref(dir);
            goto End;
        }
        struct slist_e
        { char *path;
          unsigned priority_1, priority_2, priority_3;
        };
        GSList *files = null;
        char *dir_name;
        GHashTableIter iter;
        g_hash_table_iter_init( &iter, dir_globs );
        while( g_hash_table_iter_next( &iter, ( void ** )&dir_name, ( void ** )&hash_e ))
        {   _Bool no_glob = no;
            GFile *dir_2;
            if( !strpbrk( dir_name, "*?" ))
            {   dir_2 = g_file_get_child( dir, dir_name );
                no_glob = yes;
                goto No_glob;
            }
            char **dir_names = g_strsplit( dir_name, G_DIR_SEPARATOR_S, 0 );
            unsigned dir_names_n = g_strv_length( dir_names );
            if( !dir_names_n )
            {   g_strfreev( dir_names );
                continue;
            }
            GFileEnumerator **dir_enums = g_newa( GFileEnumerator *, dir_names_n );
            unsigned dir_names_i = 0;
            unsigned dir_i = ~0U;
            dir_2 = g_file_dup(dir);
            O{  if( !strpbrk( dir_names[ dir_names_i ], "*?" ))
                {   dir_enums[ dir_names_i ] = 0;
                    GFile *dir_2_ = g_file_get_child( dir_2, dir_names[ dir_names_i ] );
                    g_object_unref( dir_2 );
                    dir_2 = dir_2_;
                    goto Single_no_glob;
                }
                if( !( dir_enums[ dir_names_i ] = g_file_enumerate_children( dir_2, G_FILE_ATTRIBUTE_STANDARD_NAME "," G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_QUERY_INFO_NONE, null, &error )))
                {   for( unsigned i = 0; i != dir_names_i; i++ )
                        if( dir_enums[i] )
                            g_object_unref( dir_enums[i] );
                    g_strfreev( dir_names );
                    g_object_unref( dir_2 );
                    g_slist_free_full( files, H_ocq_E_geany_I_open_directory_I_slist_free1 );
                    if( global_globs )
                        g_ptr_array_unref( global_globs );
                    g_hash_table_unref( dir_globs );
                    g_object_unref(dir);
                    goto End;
                }
                GPatternSpec *pattern = g_pattern_spec_new( dir_names[ dir_names_i ] );
                O{  GFileInfo *file_info = g_file_enumerator_next_file( dir_enums[ dir_names_i ], null, &error );
                    if( !file_info )
                    {   g_object_unref( dir_enums[ dir_names_i ] );
                        g_pattern_spec_free(pattern);
                        if( !dir_names_i )
                            break;
                        dir_names_i--;
                        GFile *dir_2_ = g_file_get_parent( dir_2 );
                        g_object_unref( dir_2 );
                        dir_2 = dir_2_;
                        break;
                    }
                    if( g_file_info_get_file_type( file_info ) != G_FILE_TYPE_DIRECTORY )
                    {   g_object_unref( file_info );
                        continue;
                    }
                    dir_i++;
                    const char *filename_ = g_file_info_get_name( file_info );
                    char *filename = utils_get_utf8_from_locale( filename_ );
                    g_object_unref( file_info );
                    if( !g_pattern_spec_match_string( pattern, filename ))
                    {   g_free(filename);
                        continue;
                    }
                    GFile *dir_2_ = g_file_get_child( dir_2, filename );
                    g_free(filename);
                    g_object_unref( dir_2 );
                    dir_2 = dir_2_;
Single_no_glob:     if( ++dir_names_i != dir_names_n )
                    {   if( dir_enums[ dir_names_i - 1 ] )
                            g_pattern_spec_free(pattern);
                        break;
                    }
No_glob:            ;GFileEnumerator *dir_enum;
                    if( !( dir_enum = g_file_enumerate_children( dir_2, G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NONE, null, &error )))
                    {   if( error->code == G_IO_ERROR_NOT_FOUND )
                        {   g_clear_error( &error );
                            goto Dir_not_found;
                        }
                        if( !no_glob )
                        {   if( dir_enums[ dir_names_i - 1 ] )
                                g_pattern_spec_free(pattern);
                            for( unsigned i = 0; i != dir_names_i; i++ )
                                if( dir_enums[i] )
                                    g_object_unref( dir_enums[i] );
                            g_strfreev( dir_names );
                        }
                        g_object_unref( dir_2 );
                        g_slist_free_full( files, H_ocq_E_geany_I_open_directory_I_slist_free1 );
                        if( global_globs )
                            g_ptr_array_unref( global_globs );
                        g_hash_table_unref( dir_globs );
                        g_object_unref(dir);
                        goto End;
                    }
                    while( file_info = g_file_enumerator_next_file( dir_enum, null, &error ))
                    {   const char *filename_ = g_file_info_get_name( file_info );
                        char *filename = utils_get_utf8_from_locale( filename_ );
                        g_object_unref( file_info );
                        unsigned filename_l = strlen(filename);
                        char *filename_rev = g_utf8_strreverse( filename, filename_l );
                        unsigned glob_i;
                        for( glob_i = 0; glob_i != hash_e->globs->len; glob_i++ )
                        {   GPatternSpec *file_glob = g_ptr_array_index( hash_e->globs, glob_i );
                            if( g_pattern_spec_match( file_glob, filename_l, filename, filename_rev ))
                            {   file = g_file_get_child( dir_2, filename );
                                char *path = g_file_get_path(file);
                                g_object_unref(file);
                                struct slist_e *slist_e = g_newa( struct slist_e, 1 );
                                slist_e->path = path;
                                slist_e->priority_1 = hash_e->priority_1;
                                slist_e->priority_2 = no_glob ? ~0U : dir_i;
                                slist_e->priority_3 = glob_i;
                                files = g_slist_insert_sorted_with_data( files, slist_e, ( void * )H_ocq_E_geany_I_open_directory_I_files_slist_cmp, null );
                                break;
                            }
                        }
                        if( global_globs
                        && glob_i == hash_e->globs->len
                        ){  for( glob_i = 0; glob_i != global_globs->len; glob_i++ )
                            {   GPatternSpec *global_glob = g_ptr_array_index( global_globs, glob_i );
                                if( g_pattern_spec_match( global_glob, filename_l, filename, filename_rev ))
                                {   file = g_file_get_child( dir_2, filename );
                                    char *path = g_file_get_path(file);
                                    g_object_unref(file);
                                    struct slist_e *slist_e = g_newa( struct slist_e, 1 );
                                    slist_e->path = path;
                                    slist_e->priority_1 = hash_e->priority_1;
                                    slist_e->priority_2 = no_glob ? ~0U : dir_i;
                                    slist_e->priority_3 = ~0U >> 1;
                                    files = g_slist_insert_sorted_with_data( files, slist_e, ( void * )H_ocq_E_geany_I_open_directory_I_files_slist_cmp, null );
                                    break;
                                }
                            }
                        }
                        g_free( filename_rev );
                        g_free(filename);
                    }
                    g_object_unref( dir_enum );
                    if(error)
                    {   if( !no_glob )
                        {   if( dir_enums[ dir_names_i - 1 ] )
                                g_pattern_spec_free(pattern);
                            for( unsigned i = 0; i != dir_names_i; i++ )
                                if( dir_enums[i] )
                                    g_object_unref( dir_enums[i] );
                            g_strfreev( dir_names );
                        }
                        g_object_unref( dir_2 );
                        g_slist_free_full( files, H_ocq_E_geany_I_open_directory_I_slist_free1 );
                        if( global_globs )
                            g_ptr_array_unref( global_globs );
                        g_hash_table_unref( dir_globs );
                        g_object_unref(dir);
                        goto End;
                    }
Dir_not_found:      ;
                    if( no_glob )
                        goto No_glob_end;
                    if( !dir_enums[ --dir_names_i ] )
                    {   if( dir_enums[ dir_names_i ] )
                            g_pattern_spec_free(pattern);
                        while( dir_names_i )
                        {   dir_names_i--;
                            dir_2_ = g_file_get_parent( dir_2 );
                            g_object_unref( dir_2 );
                            dir_2 = dir_2_;
                            if( dir_enums[ dir_names_i ] )
                                break;
                        }
                        if( !dir_names_i
                        && !dir_enums[ dir_names_i ]
                        )
                            break;
                        pattern = g_pattern_spec_new( dir_names[ dir_names_i ] );
                    }
                    dir_2_ = g_file_get_parent( dir_2 );
                    g_object_unref( dir_2 );
                    dir_2 = dir_2_;
                }
                if( !dir_names_i )
                    break;
            }
            g_strfreev( dir_names );
No_glob_end:g_object_unref( dir_2 );
        }
        if( global_globs )
            g_ptr_array_unref( global_globs );
        g_hash_table_unref( dir_globs );
        for( GSList *files_ = files; files_; files_ = g_slist_next( files_ ))
        {   struct slist_e *slist_e = files_->data;
            char *s = utils_get_locale_from_utf8( slist_e->path );
            g_free( slist_e->path );
            files_->data = s;
        }
        document_open_files( files, no, null, null );
        g_slist_free_full( files, g_free );
End:    if(error)
        {   ui_set_statusbar( yes, "%s", error->message );
            g_error_free(error);
        }
    }
    gtk_widget_destroy(dialog);
}
//==============================================================================
static
void
H_ocq_E_compile_I_exec_X_watch( GPid pid
, int exit_status
, void *data
){  g_spawn_close_pid( H_ocq_E_compile_S_pid );
    H_ocq_E_compile_S_pid = ( GPid )empty;
    ui_progress_bar_stop();
    GError *error = null;
    if( g_spawn_check_wait_status( exit_status, &error ))
        ui_set_statusbar( yes, "spawn finished." );
    else
    {   if( error->domain == G_SPAWN_EXIT_ERROR )
            ui_set_statusbar( yes, "spawn exited with code: %d.", error->code );
        else
            ui_set_statusbar( yes, "spawn failed." );
        g_error_free(error);
    }
    if( H_ocq_E_compile_I_exec_X_watch_S_log_Z_message )
    {   msgwin_switch_tab( MSG_MESSAGE, yes );
        keybindings_send_command( GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR );
    }else if( H_ocq_E_compile_I_exec_X_watch_S_log_Z_compiler )
    {   msgwin_switch_tab( MSG_COMPILER, yes );
        keybindings_send_command( GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR );
    }
    //NDFN Brak dostępu do zmiennej.
    //if( prefs.beep_on_errors )
        gdk_display_beep( gdk_display_get_default() );
    g_mutex_unlock( &H_ocq_E_geany_Q_action_S_mutex );
}
static
gboolean
H_ocq_E_compile_I_exec_Q_stdout_X_watch( GIOChannel *src
, GIOCondition cond
, void *data
){  if( cond == G_IO_HUP )
    {   g_io_channel_shutdown( H_ocq_E_compile_S_channel_out, no, null );
        g_io_channel_unref( H_ocq_E_compile_S_channel_out );
        return no;
    }
    int nl_length;
    g_io_channel_get_line_term( H_ocq_E_compile_S_channel_out, &nl_length );
    if( !nl_length )
        nl_length = 1;
    char *s;
    gsize l;
    if( g_io_channel_read_line( H_ocq_E_compile_S_channel_out
        , &s
        , &l
        , null
        , null
        ) == G_IO_STATUS_NORMAL
    )
    {   if(l)
        {   if( l > nl_length )
            {   s[ l - nl_length ] = '\0';
                msgwin_compiler_add( COLOR_BLUE, "%s", s );
                H_ocq_E_compile_I_exec_X_watch_S_log_Z_compiler = yes;
            }
            g_free(s);
        }
    }
    return yes;
}
static
gboolean
H_ocq_E_compile_I_exec_Q_stderr_X_watch( GIOChannel *src
, GIOCondition cond
, void *data
){  if( cond == G_IO_HUP )
    {   g_io_channel_shutdown( H_ocq_E_compile_S_channel_err, no, null );
        g_io_channel_unref( H_ocq_E_compile_S_channel_err );
        return no;
    }
    int nl_length;
    g_io_channel_get_line_term( H_ocq_E_compile_S_channel_err, &nl_length );
    if( !nl_length )
        nl_length = 1;
    char *s;
    gsize l;
    if( g_io_channel_read_line( H_ocq_E_compile_S_channel_err
        , &s
        , &l
        , null
        , null
        ) == G_IO_STATUS_NORMAL
    )
    {   if(l)
        {   if( l > nl_length )
            {   s[ l - nl_length ] = '\0';
                enum MsgColors c;
                if( strstr( s, " error: " ))
                    c = COLOR_RED;
                else if( strstr( s, " warning: " ))
                    c = COLOR_DARK_RED;
                else
                    c = COLOR_BLACK;
                char *s_1 = s;
                if( !strncmp( s_1, "In file included from ", 22 )
                    || !strncmp( s_1, "                 from ", 22 )
                )
                    s_1 += 22;
                if( H_ocq_E_compile_I_make_S_coux_project )
                {   while( g_str_has_prefix( s_1, "../" ))
                        s_1 += 6;
                    char *s_2 = g_utf8_strchr( s_1, -1, ':' );
                    if( s_2 )
                    {   *s_2 = '\0';
                        if( g_str_has_suffix( s_1, ".c" ))
                        {   s_1 = g_strconcat( s_1, "x:", s_2 + 1, null );
                            g_free(s);
                            s = s_1;
                        }else
                            *s_2 = ':';
                    }
                }
                msgwin_compiler_add( c, "%s", s_1 );
                H_ocq_E_compile_I_exec_X_watch_S_log_Z_compiler = yes;
                if( c != COLOR_BLACK )
                {   msgwin_msg_add(
                      c
                    , -1
                    , null
                    , "%s"
                    , s_1
                    );
                    H_ocq_E_compile_I_exec_X_watch_S_log_Z_message = yes;
                }
            }
            g_free(s);
        }
    }
    return yes;
}
static
_Bool
H_ocq_E_compile_I_exec( char *argv[]
, char *change_to_directory
){  msgwin_clear_tab( MSG_COMPILER );
    msgwin_clear_tab( MSG_MESSAGE );
    msgwin_set_messages_dir( change_to_directory );
    msgwin_switch_tab( MSG_STATUS, no );
    keybindings_send_command( GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR );
    GString *s = g_string_new( "spawn:" );
    int i = -1;
    while( argv[++i] )
    {   if( argv[i][0] == '-' )
        {   if( argv[i][1] == '-'
            && argv[i][2] == '\0'
            )
                break;
            continue;
        }
        g_string_append_c( s, ' ' );
        g_string_append( s, argv[i] );
    }
    while( argv[++i] )
    {   g_string_append_c( s, ' ' );
        g_string_append( s, argv[i] );
    }
    ui_set_statusbar( yes, "%s", s->str );
    int out, err;
    if( !g_spawn_async_with_pipes( change_to_directory
        , argv
        , null
        , G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH
        , null
        , null
        , &H_ocq_E_compile_S_pid
        , null
        , &out
        , &err
        , null
        )
    )
    {   g_string_free( s, yes );
        ui_set_statusbar( yes, "spawn failed." );
        return no;
    }
    ui_progress_bar_start( s->str );
    g_string_free( s, yes );
    H_ocq_E_compile_I_exec_X_watch_S_log_Z_compiler = no;
    H_ocq_E_compile_I_exec_X_watch_S_log_Z_message = no;
    g_child_watch_add( H_ocq_E_compile_S_pid
    , H_ocq_E_compile_I_exec_X_watch
    , null
    );
    H_ocq_E_compile_S_channel_out = g_io_channel_unix_new(out);
    H_ocq_E_compile_S_channel_err = g_io_channel_unix_new(err);
    g_io_add_watch( H_ocq_E_compile_S_channel_out, G_IO_IN | G_IO_HUP, H_ocq_E_compile_I_exec_Q_stdout_X_watch, null );
    g_io_add_watch( H_ocq_E_compile_S_channel_err, G_IO_IN | G_IO_HUP, H_ocq_E_compile_I_exec_Q_stderr_X_watch, null );
    return yes;
}
//------------------------------------------------------------------------------
static
void
H_ocq_E_compile_I_make_I_save(
  GFile *dir
){  unsigned i;
    foreach_document(i)
    {   if( !documents[i]->real_path )
            continue;
        GFile *file = g_file_new_for_path( documents[i]->real_path );
        if( g_file_has_prefix( file, dir ))
        {   editor_indicator_clear( documents[i]->editor, GEANY_INDICATOR_ERROR );
            document_save_file( documents[i], no );
        }
        g_object_unref(file);
    }
}
static
_Bool
H_ocq_E_compile_I_make(
  char *target
){  GeanyDocument *document = document_get_current();
    if( !document
    || !document->real_path
    )
        return no;
    GFile *dir_1 = null, *dir = g_file_new_for_path( document->real_path );
    GFile *parent = g_file_get_parent(dir);
    // Poniżej – jeśli polecenie wydane jest z głównego katalogu użytkownika, to limit podążania w hierarchii systemu plików do poniżej takiego katalogu jest intencjonalnie ignorowany.
    GFile *min_parent_dir = null;
    if(parent)
    {   min_parent_dir = g_file_new_for_path( g_get_home_dir() );
        if( !g_file_has_prefix( parent, min_parent_dir ))
            g_clear_object( &min_parent_dir );
    }
    _Bool b_loop = no;
    O{  g_object_unref(dir);
        dir = parent;
        if( !dir )
            break;
        if( min_parent_dir
        && g_file_equal( dir, min_parent_dir )
        ){  g_clear_object( &dir );
            break;
        }
        GFile *makefile = g_file_get_child( dir, "Makefile" );
        GError *error = null;
        GFileInfo *file_info = g_file_query_info( makefile, G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_QUERY_INFO_NONE, null, &error );
        g_object_unref(makefile);
        if(error)
        {   if( error->code != G_IO_ERROR_NOT_FOUND )
            {   g_object_unref( min_parent_dir );
                g_object_unref(dir);
                char *s = g_file_get_path(makefile);
                ui_set_statusbar( yes, "%s: %s", error->message, s );
                g_free(s);
                g_error_free(error);
                return no;
            }
            g_error_free(error);
        }else
        {   if( g_file_info_get_file_type( file_info ) == G_FILE_TYPE_REGULAR )
                g_set_object( &dir_1, dir );
            g_object_unref( file_info );
        }
        parent = g_file_get_parent(dir);
        b_loop = yes;
        break;
    }
    if( b_loop )
        O{  g_object_unref(dir);
            dir = parent;
            if( !dir )
                break;
            if( min_parent_dir
            && g_file_equal( dir, min_parent_dir )
            ){  g_clear_object( &dir );
                break;
            }
            GFile *makefile = g_file_get_child( dir, "Makefile" );
            GError *error = null;
            GFileInfo *file_info = g_file_query_info( makefile, G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_QUERY_INFO_NONE, null, &error );
            g_object_unref(makefile);
            if(error)
            {   if( error->code != G_IO_ERROR_NOT_FOUND )
                {   g_object_unref( min_parent_dir );
                    g_object_unref(dir);
                    char *s = g_file_get_path(makefile);
                    ui_set_statusbar( yes, "%s: %s", error->message, s );
                    g_free(s);
                    g_error_free(error);
                    return no;
                }
                g_error_free(error);
            }else
            {   if( g_file_info_get_file_type( file_info ) == G_FILE_TYPE_REGULAR )
                {   g_object_unref( file_info );
                    break;
                }
                g_object_unref( file_info );
            }
            parent = g_file_get_parent(dir);
        }
    if( min_parent_dir )
        g_object_unref( min_parent_dir );
    if( !dir
    && !dir_1
    )
        return no;
    H_ocq_E_compile_I_make_I_save( dir_1 ? dir_1 : dir );
    if( GPOINTER_TO_UINT(target) != (unsigned)empty )
    {   char *dir_path = g_file_get_path( dir_1 ? dir_1 : dir );
        if( dir_1 )
            g_object_unref( dir_1 );
        if(dir)
            g_object_unref(dir);
        H_ocq_E_compile_I_make_S_coux_project = g_str_has_suffix( document->file_name, ".cx" );
        _Bool ret = H_ocq_E_compile_I_exec(( char *[] ){ "make", "-s", "--", target, null }
        , dir_path
        );
        g_free( dir_path );
        return ret;
    }
    if(dir)
        g_object_unref(dir);
    return yes;
}
//==============================================================================
static
void
H_ocq_E_geany_I_show_status_tab( void
){  msgwin_switch_tab( MSG_STATUS, yes );
    keybindings_send_command( GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR );
}
static
void
H_ocq_E_geany_I_show_doc_com_tab( void
){  msgwin_switch_tab( MSG_DOC_COM, yes );
    keybindings_send_command( GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR );
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///NDFN Tabulatory nie zamieniane na odstępy.
static
void
H_ocq_E_geany_I_export_to_html( void
){  GeanyDocument *document = document_get_current();
    if( !document )
        return;
    int length = sci_get_length( document->editor->sci );
    if( !length )
        return;
    GHashTable *uni_style = g_hash_table_new( null, null );
    const GeanyLexerStyle *gls = highlighting_get_style( GEANY_FILETYPES_C, SCE_C_DEFAULT );
    int def_foreground = gls->foreground;
    g_print( "foreground: %x\n", gls->foreground );
    gls = highlighting_get_style( GEANY_FILETYPES_C, SCE_C_COMMENT );
    g_hash_table_insert( uni_style, GINT_TO_POINTER( gls->foreground ), GINT_TO_POINTER( 1 ));
    //g_hash_table_insert( uni_style, GINT_TO_POINTER( 0x5f5f5f ), GINT_TO_POINTER( 1 ));
    gls = highlighting_get_style( GEANY_FILETYPES_C, SCE_C_COMMENTDOC );
    g_hash_table_insert( uni_style, GINT_TO_POINTER( gls->foreground ), GINT_TO_POINTER( 2 ));
    //g_hash_table_insert( uni_style, GINT_TO_POINTER( 0x5f372b ), GINT_TO_POINTER( 2 ));
    g_hash_table_insert( uni_style, GINT_TO_POINTER( 0x00005f ), GINT_TO_POINTER( 3 ));
    gls = highlighting_get_style( GEANY_FILETYPES_C, 7 );
    g_hash_table_insert( uni_style, GINT_TO_POINTER( gls->foreground ), GINT_TO_POINTER( 4 ));
    //g_hash_table_insert( uni_style, GINT_TO_POINTER( 0x5f0000 ), GINT_TO_POINTER( 4 ));
    gls = highlighting_get_style( GEANY_FILETYPES_C, 14 );
    g_hash_table_insert( uni_style, GINT_TO_POINTER( gls->foreground ), GINT_TO_POINTER( 5 ));
    //g_hash_table_insert( uni_style, GINT_TO_POINTER( 0x002b00 ), GINT_TO_POINTER( 5 ));
    gls = highlighting_get_style( GEANY_FILETYPES_C, 8 );
    g_hash_table_insert( uni_style, GINT_TO_POINTER( gls->foreground ), GINT_TO_POINTER( 6 ));
    //g_hash_table_insert( uni_style, GINT_TO_POINTER( 0x005f00 ), GINT_TO_POINTER( 6 ));
    gls = highlighting_get_style( GEANY_FILETYPES_C, 6 );
    g_hash_table_insert( uni_style, GINT_TO_POINTER( gls->foreground ), GINT_TO_POINTER( 7 ));
    //g_hash_table_insert( uni_style, GINT_TO_POINTER( 0x37005f ), GINT_TO_POINTER( 7 ));
    gls = highlighting_get_style( GEANY_FILETYPES_C, 9 );
    g_hash_table_insert( uni_style, GINT_TO_POINTER( gls->foreground ), GINT_TO_POINTER( 8 ));
    //g_hash_table_insert( uni_style, GINT_TO_POINTER( 0x00375f ), GINT_TO_POINTER( 8 ));
    GString *s_out = g_string_new( "<pre><!--" );
    char *s = DOC_FILENAME(document);
    char *s_ = strrchr( s, '/' );
    if( s_ )
        s_++;
    else
        s_ = s;
    g_string_append( s_out, s_ );
    g_string_append( s_out, "-->" );
    _Bool inside_span = no;
    int st = STYLE_DEFAULT;
    char uc[7];
    int uc_i = 0;
    char byte;
    for( int i = 0; i < length; i++ )
    {   byte = sci_get_char_at( document->editor->sci, i );
        if(( byte & 0x80 ) == 0 // Pierwszy bajt następnego znaku.
        || uc_i == 5 // Ostatni bajt bieżącego znaku.
        )
        {   if( uc_i )
            {   int st_ = sci_get_style_at( document->editor->sci, i - uc_i );
                if( uc_i == 5 )
                    uc[ uc_i++ ] = byte;
                uc[ uc_i ] = '\0';
                gunichar c = g_utf8_get_char( &uc[0] );
                if( inside_span
                    && st_ != st
                )
                {   g_string_append( s_out, "</span>" );
                    inside_span = no;
                    st = STYLE_DEFAULT;
                }
                if( st_ != st
                    && !g_unichar_isspace(c)
                )
                {   int fg = scintilla_send_message( document->editor->sci, SCI_STYLEGETFORE, st_, 0 );
                    if( fg != def_foreground )
                    {   void *p = g_hash_table_lookup( uni_style, GINT_TO_POINTER( fg ));
                        g_string_append_printf( s_out, "<span class=\"%c\">", 'A' + GPOINTER_TO_INT(p) - 1 );
                        inside_span = yes;
                    }
                    st = st_;
                }
                char *s_;
                switch(c)
                { case L'&':
                        s_ = "&amp;";
                        break;
                  case L'<':
                        s_ = "&lt;";
                        break;
                  case L'>':
                        s_ = "&gt;";
                        break;
                  default:
                        s_ = &uc[0];
                        break;
                }
                g_string_append( s_out, s_ );
            }
            if( uc_i <= 4 )
            {   int st_ = sci_get_style_at( document->editor->sci, i );
                if( inside_span
                && st_ != st
                ){  g_string_append( s_out, "</span>" );
                    inside_span = no;
                    st = STYLE_DEFAULT;
                }
                if( st_ != st
                && !isspace(byte)
                ){  int fg = scintilla_send_message( document->editor->sci, SCI_STYLEGETFORE, st_, 0 );
                    if( fg != def_foreground )
                    {   void *p = g_hash_table_lookup( uni_style, GINT_TO_POINTER( fg ));
                        g_string_append_printf( s_out, "<span class=\"%c\">", 'A' + GPOINTER_TO_INT(p) - 1 );
                        inside_span = yes;
                    }
                    st = st_;
                }
                char *s_ = null;
                switch(byte)
                { case '&':
                        s_ = "&amp;";
                        break;
                  case '<':
                        s_ = "&lt;";
                        break;
                  case '>':
                        s_ = "&gt;";
                        break;
                }
                if( s_ )
                    g_string_append( s_out, s_ );
                else
                    g_string_append_c( s_out, byte );
            }
            uc_i = 0;
        }else
            uc[ uc_i++ ] = byte;
    }
    if( uc_i )
    {   int st_ = sci_get_style_at( document->editor->sci, length - uc_i );
        if( uc_i == 5 )
            uc[ uc_i++ ] = byte;
        uc[ uc_i ] = '\0';
        gunichar c = g_utf8_get_char( &uc[0] );
        if( inside_span
        && st_ != st
        ){  g_string_append( s_out, "</span>" );
            inside_span = no;
            st = STYLE_DEFAULT;
        }
        if( st_ != st
        && !g_unichar_isspace(c)
        ){  int fg = scintilla_send_message( document->editor->sci, SCI_STYLEGETFORE, st_, 0 );
            if( fg != def_foreground )
            {   void *p = g_hash_table_lookup( uni_style, GINT_TO_POINTER( fg ));
                g_string_append_printf( s_out, "<span class=\"%c\">", 'A' + GPOINTER_TO_INT(p) - 1 );
                inside_span = yes;
            }
            st = st_;
        }
        char *s_;
        switch(c)
        { case L'&':
                s_ = "&amp;";
                break;
          case L'<':
                s_ = "&lt;";
                break;
          case L'>':
                s_ = "&gt;";
                break;
          default:
                s_ = &uc[0];
                break;
        }
        g_string_append( s_out, s_ );
    }
    if( inside_span )
        g_string_append( s_out, "</span>" );
    g_string_append( s_out, "</pre>\n" );
    document = document_new_file( null, filetypes[ GEANY_FILETYPES_HTML ], s_out->str );
    document_set_text_changed( document, yes );
    g_string_free( s_out, yes );
    g_hash_table_destroy( uni_style );
    return;
}
//------------------------------------------------------------------------------
static
void
H_ocq_E_geany_I_restyle_source_text( void
){  const int style_line_n = 80;
    GeanyDocument *document = document_get_current();
    if( !document )
        return;
    if( strcmp( document->encoding, encodings_get_charset_from_index( GEANY_ENCODING_UTF_8 )))
        return;
    ScintillaObject *sci = document->editor->sci;
    const char *eol = editor_get_eol_char( document->editor );
    sci_start_undo_action(sci);
    for( int line = 0; line < sci_get_line_count(sci); line++ )
    {   char *s_0 = sci_get_line( sci, line );
        int i, n_0, l_0;
        if( !strncmp( s_0, "#", i = n_0 = l_0 = 1 )
        || !strncmp( s_0, "//", i = n_0 = l_0 = 2 )
        || !strncmp( s_0, "\'", i = n_0 = l_0 = 1 )
        ){  char *s = s_0 + i;
            if( strcmp( s, eol ))
            {   gunichar c_1 = g_utf8_get_char(s), c = c_1;
                i++;
                while(( s = g_utf8_next_char(s) ), g_utf8_get_char(s) == c )
                    i++;
                if(( !strcmp( s, eol )
                  || *s == '\0'
                ) && i != style_line_n
                ){  int pos = sci_get_position_from_line( sci, line );
                    sci_set_selection_start( sci, pos );
                    sci_set_selection_end( sci, sci_get_line_end_position( sci, line ));
                    char s_1[6];
                    int l_1 = g_unichar_to_utf8( c_1, s_1 );
                    char *s_buf = g_malloc( l_0 + l_1 * ( style_line_n - n_0 ) + 1 );
                    strncpy( s_buf, s_0, l_0 );
                    char *s_end;
                    for( s = s_buf + l_0, s_end = s + l_1 * ( style_line_n - n_0 ); s != s_end; s += l_1 )
                        strncpy( s, s_1, l_1 );
                    *s = '\0';
                    sci_replace_sel( sci, s_buf );
                    g_free( s_buf );
                }
            }
        }
        //sci_set_current_position( sci, pos + 3, no );
        g_free( s_0 );
    }
    sci_end_undo_action(sci);
    return;
}
//------------------------------------------------------------------------------
struct H_ocq_E_geany_I_zen_snippet_Z_html_element
{   GPtrArray *name;
    _Bool id;
    _Bool class;
};
/** Stan implementacji:
e#
e.
e[a]
    e+e
    e*n
e>e
*/
static
void
H_ocq_E_geany_I_zen_snippet( void
){  GeanyDocument *document = document_get_current();
    if( !document )
        return;
    if( strcmp( document->encoding, encodings_get_charset_from_index( GEANY_ENCODING_UTF_8 )))
        return;
    ScintillaObject *s = document->editor->sci;
    int current_line = sci_get_current_line(s);
    int line_pos = sci_get_position_from_line( s, current_line );
    int current_pos = sci_get_current_position(s);
    char *in = sci_get_contents_range( s
    , line_pos
    , current_pos
    );
    int new_cursor_pos = 1;
    _Bool normal_end = no;
    _Bool parse_error = no;
    struct H_ocq_E_geany_I_zen_snippet_Z_html_element e =
    { null
    , no
    , no
    };
    GString *out = g_string_new("");
    char *pos = in + strlen(in);
    while( pos > in )
    {   pos = g_utf8_prev_char(pos);
        gunichar c = g_utf8_get_char(pos);
        switch(c)
        { case '#':
                if( e.id )
                {   parse_error = yes;
                    break;
                }
                e.id = yes;
                break;
          case '.':
                if( e.class )
                {   parse_error = yes;
                    break;
                }
                e.class = yes;
                break;
          case ']':
                *pos = '\0';
                while( pos > in )
                {   pos = g_utf8_prev_char(pos);
                    gunichar c = g_utf8_get_char(pos);
                    if( !g_unichar_isalpha(c) )
                    {   if( c != '[' )
                        {   parse_error = yes;
                            break;
                        }
                        break;
                    }
                }
                if( parse_error )
                    break;
                if( pos == in )
                {   parse_error = yes;
                    break;
                }
                if( e.name )
                {   for( guint i = 0; i < e.name->len; i++ )
                        if( !strcmp( e.name->pdata[i], pos + 1 ))
                        {   parse_error = yes;
                            break;
                        }
                    if( parse_error )
                        break;
                }else
                    e.name = g_ptr_array_new();
                g_ptr_array_add( e.name, pos + 1 );
                break;
          case '>':
                break;
          case ' ':
          case '\t':
                pos++;
                normal_end = yes;
                break;
          default:
                if( g_unichar_isalpha(c) )
                {   int i = 0;
                    for( char *out_str = out->str
                    ; *out_str
                    ; out_str = g_utf8_next_char( out_str )
                    )
                    {   if( *out_str == '\n' )
                        {   gsize new_str = out_str - out->str + 1;
                            g_string_insert_c( out, new_str, '\t' );
                            out_str = out->str + new_str;
                            if( i <= new_cursor_pos )
                                new_cursor_pos++;
                        }
                        i++;
                    }
                    g_string_prepend( out, ">\n\t" );
                    new_cursor_pos += 2;
                    if( e.name )
                    {   for( guint i = 0; i < e.name->len; i++ )
                        {   g_string_prepend( out, "=\"\"" );
                            new_cursor_pos += 3;
                            g_string_prepend( out, e.name->pdata[i] );
                            new_cursor_pos += g_utf8_strlen( e.name->pdata[i], -1 );
                            g_string_prepend_c( out, ' ' );
                            new_cursor_pos++;
                        }
                        g_ptr_array_free( e.name, yes );
                    }
                    if( e.class )
                    {   g_string_prepend( out, " class=\"\"" );
                        new_cursor_pos += 9;
                    }
                    if( e.id )
                    {   g_string_prepend( out, " id=\"\"" );
                        new_cursor_pos += 6;
                    }
                    e = ( struct H_ocq_E_geany_I_zen_snippet_Z_html_element )
                    { null
                    , no
                    , no
                    };
                    pos = g_utf8_next_char(pos);
                    *pos = '\0';
                    while( pos > in )
                    {   pos = g_utf8_prev_char(pos);
                        gunichar c = g_utf8_get_char(pos);
                        if( !g_unichar_isalpha(c) )
                        {   pos = g_utf8_next_char(pos);
                            break;
                        }
                    }
                    g_string_prepend( out, pos );
                    new_cursor_pos += g_utf8_strlen( pos, -1 );
                    g_string_prepend_c( out, '<' );
                    new_cursor_pos++;
                    g_string_append( out, "\n</" );
                    g_string_append( out, pos );
                    g_string_append_c( out, '>' );
                    break;
                }
                pos = g_utf8_next_char(pos);
                parse_error = yes;
                break;
        }
        if( parse_error
        || normal_end
        )
            break;
    }
    if( !out->len )
        parse_error = yes;
    if( e.name )
        g_ptr_array_free( e.name, yes );
    if( !parse_error )
    {   int insert_pos = line_pos + g_utf8_pointer_to_offset( in, pos );
        sci_start_undo_action(s);
        sci_set_selection_start( s, insert_pos );
        sci_set_selection_end( s, current_pos );
        sci_replace_sel( s, "" );
        editor_insert_text_block( document->editor
        , out->str
        , insert_pos
        , new_cursor_pos
        , -1
        , yes
        );
        sci_end_undo_action(s);
    }
    g_free(in);
    g_string_free( out, yes );
    return;
}
//==============================================================================
static
void
H_ocq_E_doc_com_I_idle_update( void
){  GeanyDocument *document = document_get_current();
    if( !document )
        return;
    if( strcmp( document->encoding, encodings_get_charset_from_index( GEANY_ENCODING_UTF_8 )))
        return;
    ScintillaObject *sci = document->editor->sci;
    GtkTreeModel *list_store = gtk_tree_view_get_model(( void * )H_ocq_E_doc_com_S_page );
    gtk_list_store_clear(( void * )list_store );
    int current_line = sci_get_current_line(sci);
    int lexer = sci_get_lexer(sci);
    struct Sci_TextToFind sttf;
    long last_start = sttf.chrg.cpMin = 0;
    sttf.chrg.cpMax = sci_get_length(sci);
    sttf.lpstrText = "[/#'<]";
    while( sci_find_text( sci, SCFIND_MATCHCASE | SCFIND_REGEXP, &sttf ) != -1 )
    {   if( !highlighting_is_comment_style( lexer, sci_get_style_at( sci, sttf.chrgText.cpMin ))
        || ( sttf.chrgText.cpMin
          && highlighting_is_comment_style( lexer, sci_get_style_at( sci, sttf.chrgText.cpMin - 1 ))
          && sttf.chrg.cpMin != last_start
        ))
        {   sttf.chrg.cpMin = sttf.chrgText.cpMax;
            goto Next;
        }
        int line = sci_get_line_from_position( sci, sttf.chrgText.cpMin );
        int cnt_end = sci_get_line_end_position( sci, line );
        _Bool rep_bind = yes;
        char c = sci_get_char_at( sci, sttf.chrgText.cpMin );
        if( c == '/'
        || c == '<'
        )
        {   struct Sci_TextToFind sttf_;
            if( c == '/' )
            {   c = sci_get_char_at( sci, sttf.chrgText.cpMax );
                if( c == '/' )
                {   sttf.chrgText.cpMax++;
                    goto Cont_line;
                }
                if( c != '*' )
                {   sttf.chrg.cpMin = sttf.chrgText.cpMax;
                    goto Next;
                }
                sttf.chrgText.cpMax++;
                sttf_.lpstrText = "*/";
            }else
            {   char *s_ = sci_get_contents_range( sci, sttf.chrgText.cpMax, sttf.chrgText.cpMax + 3 );
                if( strcmp( s_, "!--" ))
                {   g_free( s_ );
                    sttf.chrg.cpMin = sttf.chrgText.cpMax;
                    goto Next;
                }
                g_free( s_ );
                rep_bind = no;
                sttf.chrgText.cpMax += 3;
                sttf_.lpstrText = "-->";
            }
            sttf_.chrg.cpMin = sttf.chrgText.cpMax;
            sttf_.chrg.cpMax = sttf.chrg.cpMax;
            if( sci_find_text( sci, SCFIND_MATCHCASE, &sttf_ ) == -1 )
                break;
            sttf.chrg.cpMin = sttf_.chrgText.cpMax;
            if( cnt_end > sttf_.chrgText.cpMin )
                cnt_end = sttf_.chrgText.cpMin;
        }else
        {
Cont_line:  sttf.chrg.cpMin = cnt_end + editor_get_eol_char_len( document->editor );
        }
        if( sttf.chrgText.cpMax == cnt_end )
            goto Next;
        char *s = sci_get_contents_range( sci, sttf.chrgText.cpMax, cnt_end );
        char *p = s;
        gunichar u;
        _Bool type_doc = no;
        if( rep_bind )
        {   u = g_utf8_get_char(p);
            if( u == c )
            {   type_doc = yes;
                p = g_utf8_next_char(p);
            }else if( g_unichar_isspace(u) )
                p = g_utf8_next_char(p);
            while( *p
            && (( u = g_utf8_get_char(p) ) == c
              || g_unichar_isspace(u)
            ))
                p = g_utf8_next_char(p);
        }else
            while( *p
            && g_unichar_isspace( g_utf8_get_char(p) )
            )
                p = g_utf8_next_char(p);
        char *text_orig = p;
        while( *p
        && g_unichar_ispunct( g_utf8_get_char(p) )
        )
            p = g_utf8_next_char(p);
        if( !*p )
            goto Next_s;
        char *text = p;
        if( rep_bind )
        {   p = g_utf8_prev_char( s + ( cnt_end - sttf.chrgText.cpMax ));
            if(( u = g_utf8_get_char(p) ) == c
            || g_unichar_isspace(u)
            ){  if( p == text )
                    goto Next_s;
                char *p_;
                while(( u = g_utf8_get_char( p = g_utf8_prev_char( p_ = p ))) == c
                || g_unichar_isspace(u)
                ){  if( p == text )
                        goto Next_s;
                }
                *p_ = '\0';
            }
        }
        p = text;
        O{  if( !*p )
                goto Next_s;
            if( !g_unichar_ispunct( g_utf8_get_char(p) ))
                break;
            p = g_utf8_next_char(p);
        }
        char *tag = "";
        unsigned i = 0;
        _Bool tag_weak = no;
        while( g_unichar_isalpha( u = g_utf8_get_char(p) )
        && ( !tag_weak && !g_unichar_isupper(u) && ( tag_weak = yes ), yes )
        && ( p = g_utf8_next_char(p), ++i != 5 )
        && *p
        ){}
        if( i
        && i != 5
        )
        {   if( *p )
            {   if( g_unichar_ispunct(u) && u != '_'
                || g_unichar_isspace(u)
                ){  char *tag_0 = p;
                    while( g_unichar_ispunct( u = g_utf8_get_char(p) ) && u != '_'
                    && *( p = g_utf8_next_char(p) )
                    ){}
                    while( g_unichar_isspace( g_utf8_get_char(p) )
                    && *( p = g_utf8_next_char(p) )
                    ){}
                    if(( *p
                      && p != tag_0
                    ) || !tag_weak
                    ){  tag = text;
                        *tag_0 = '\0';
                        text = p;
                    }else
                        text = text_orig;
                }else
                    text = text_orig;
            }else if( !tag_weak )
            {   tag = text;
                *p = '\0';
                text = p;
            }else
                text = text_orig;
        }else
            text = text_orig;
        GtkTreeIter iter;
        gtk_list_store_insert_with_values(( void * )list_store
        , &iter, -1
        , 0, line + 1
        , 1, type_doc
        , 2, tag
        , 3, text
        , -1
        );
        if( line == current_line )
        {   GtkTreePath *path = gtk_tree_model_get_path( list_store, &iter );
            gtk_tree_view_set_cursor(( void * )H_ocq_E_doc_com_S_page, path, null, no );
            gtk_tree_path_free(path);
        }
Next_s: g_free(s);
Next:   if( sttf.chrg.cpMin >= sttf.chrg.cpMax )
            break;
        last_start = sttf.chrg.cpMin;
    }
}
static
gboolean
H_ocq_E_doc_com_I_idle_update_I(
  void *data
){  H_ocq_E_doc_com_I_idle_update();
    H_ocq_E_doc_com_I_idle_update_S = 0;
    return G_SOURCE_REMOVE;
}
static
void
H_ocq_E_doc_com_I_idle_update_M( void
){  if( !H_ocq_E_doc_com_I_idle_update_S )
        H_ocq_E_doc_com_I_idle_update_S = g_idle_add( H_ocq_E_doc_com_I_idle_update_I, null );
}
static
void
H_ocq_E_doc_com_I_idle_update_W( void
){  if( H_ocq_E_doc_com_I_idle_update_S )
    {   g_source_remove( H_ocq_E_doc_com_I_idle_update_S );
        H_ocq_E_doc_com_I_idle_update_S = 0;
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static
void
H_ocq_E_doc_com_X_row_activated( GtkTreeView *tree_view
, GtkTreePath *tree_path
, GtkTreeViewColumn *column
, void *data
){  GtkTreeModel *model = gtk_tree_view_get_model( tree_view );
    GtkTreeIter iter;
    gtk_tree_model_get_iter( model, &iter, tree_path );
    int line;
    gtk_tree_model_get( model, &iter, 0, &line, -1 );
    GeanyEditor *editor = document_get_current()->editor;
    editor_goto_pos( editor, sci_get_position_from_line( editor->sci, line - 1 ), yes );
}
//==============================================================================
static
gboolean
H_ocq_E_geany_X_editor_notify( GObject *object
, GeanyEditor *editor
, SCNotification *notification
, void *data
){  if( notification->nmhdr.code == SCN_MODIFIED
    && notification->modificationType & ( SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT )
    )
        H_ocq_E_doc_com_I_idle_update_M();
    return no;
}
//------------------------------------------------------------------------------
static
void
H_ocq_E_geany_X_document_new_open_I_doc_com_tab( void
){  unsigned i, n = 0;
    foreach_document(i)
    {   if( ++n > 1 )
            break;
    }
    if( n == 1 )
        H_ocq_E_geany_I_show_doc_com_tab();
}
static
void
H_ocq_E_geany_X_document_new( GObject *obj
, GeanyDocument *doc
, void *data
){  GeanyDocument *last_doc = document_index( H_ocq_E_geany_S_last_document );
    if( last_doc
    && last_doc->file_name // Czyli nie nowy plik po uruchomieniu programu.
    )
    {   char *s = utils_get_locale_from_utf8( last_doc->file_name );
        GFile *file = g_file_new_for_path(s);
        g_free(s);
        GFile *dir = g_file_get_parent(file);
        g_object_unref(file);
        if(dir)
        {   if( !doc->file_name )
                doc->file_name = g_strdup( "untitled" );
            char *s = utils_get_locale_from_utf8( doc->file_name );
            file = g_file_get_child( dir, s );
            g_free(s);
            g_object_unref(dir);
            GtkWidget *dialog = gtk_file_chooser_dialog_new( "new file"
            , ( void * )geany_data->main_widgets->window
            , GTK_FILE_CHOOSER_ACTION_SAVE
            , "_cancel"
            , GTK_RESPONSE_REJECT
            , "_name"
            , GTK_RESPONSE_ACCEPT
            , null
            );
            s = g_file_get_path(file);
            gtk_file_chooser_set_filename(( void * )dialog, s );
            g_free(s);
            s = g_file_get_basename(file);
            g_object_unref(file);
            gtk_file_chooser_set_current_name(( void * )dialog, s );
            g_free(s);
            GPtrArray *dirs = g_ptr_array_new();
            g_ptr_array_set_free_func( dirs, g_free );
            unsigned i;
            foreach_document(i)
            {   if( documents[i]->real_path )
                    file = g_file_new_for_path( documents[i]->real_path );
                else
                {   s = utils_get_locale_from_utf8( last_doc->file_name );
                    file = g_file_new_for_path(s);
                    g_free(s);
                }
                dir = g_file_get_parent(file);
                g_object_unref(file);
                if( !dir )
                    continue;
                s = g_file_get_path(dir);
                g_object_unref(dir);
                unsigned j = 0;
                for( ; j < dirs->len; j++ )
                    if( !strcmp( s, g_ptr_array_index( dirs, j )))
                        break;
                if( j < dirs->len )
                    g_free(s);
                else
                    g_ptr_array_add( dirs, s );
            }
            for( unsigned i = 0; i < dirs->len; i++ )
                gtk_file_chooser_add_shortcut_folder(( void * )dialog, g_ptr_array_index( dirs, i ), null );
            g_ptr_array_free( dirs, yes );
            gtk_file_chooser_set_do_overwrite_confirmation(( void * )dialog, yes );
            if( gtk_dialog_run(( void * )dialog ) == GTK_RESPONSE_ACCEPT )
            {   file = gtk_file_chooser_get_file(( void * )dialog );
                s = g_file_get_path(file);
                char *filename = utils_get_utf8_from_locale(s);
                g_free(s);
                g_free( doc->file_name );
                doc->file_name = filename;
                s = g_file_get_basename(file);
                g_object_unref(file);
                filename = utils_get_utf8_from_locale(s);
                g_free(s);
                GtkWidget *label = gtk_notebook_get_tab_label(( void * )geany_data->main_widgets->notebook, gtk_notebook_get_nth_page(( void * )geany_data->main_widgets->notebook, document_get_notebook_page(doc) ));
                GList *ch = gtk_container_get_children(( void * )label );
                label = ch->data;
                g_list_free(ch);
                GList *ch_ = gtk_container_get_children(( void * )label );
                ch = ch_;
                while( ch
                && G_OBJECT_TYPE( ch->data ) != GTK_TYPE_LABEL
                )
                    ch = ch->next;
                label = ch->data;
                g_list_free( ch_ );
                gtk_label_set_text(( void * )label, filename );
                g_free(filename);
            }
            gtk_widget_destroy(dialog);
        }
    }
    H_ocq_E_geany_X_document_new_open_I_doc_com_tab();
}
static
void
H_ocq_E_geany_X_document_open( GObject *obj
, GeanyDocument *doc
, void *data
){  H_ocq_E_geany_X_document_new_open_I_doc_com_tab();
}
//------------------------------------------------------------------------------
static
void
H_ocq_E_geany_X_document_activate( GObject *obj
, GeanyDocument *doc
, void *data
){  H_ocq_E_geany_S_last_document = H_ocq_E_geany_S_current_document;
    H_ocq_E_geany_S_current_document = doc->index;
    H_ocq_E_doc_com_I_idle_update_M();
}
static
void
H_ocq_E_geany_X_document_filetype_set( GObject *obj
, GeanyDocument *doc
, void *data
){  H_ocq_E_doc_com_I_idle_update_M();
}
static
void
H_ocq_E_geany_X_document_close( GObject *obj
, GeanyDocument *doc
, void *data
){  _Bool b = no;
    unsigned i;
    foreach_document(i)
    {   b = yes;
        break;
    }
    if(b)
    {   H_ocq_E_doc_com_I_idle_update_W();
        GtkTreeModel *list_store = gtk_tree_view_get_model(( void * )H_ocq_E_doc_com_S_page );
        gtk_list_store_clear(( void * )list_store );
    }
    char *tmp_file_templ = g_build_filename( g_get_tmp_dir(), "tmp.", null );
    unsigned tmp_file_templ_length = strlen( tmp_file_templ );
    if( doc->real_path
    && !strncmp( doc->real_path, tmp_file_templ, tmp_file_templ_length )
    && !strchr( doc->real_path + tmp_file_templ_length, G_DIR_SEPARATOR )
    ){  g_free( tmp_file_templ );
        foreach_document(i)
        {   if( i == doc->index )
                continue;
            if( documents[i]->real_path
            && !strcmp( documents[i]->real_path, doc->real_path )
            )
                return;
        }
        char *lock_file = g_strconcat( doc->real_path, ".lock", null );
        unlink( lock_file );
        g_free( lock_file );
    }else
        g_free( tmp_file_templ );
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void ( *H_ocq_E_geany_Q_action_S[] )(void) =
{ H_ocq_E_geany_I_open_directory
, H_ocq_E_geany_I_export_to_html
, H_ocq_E_geany_I_restyle_source_text
, H_ocq_E_geany_I_zen_snippet
, H_ocq_E_geany_I_show_status_tab
, H_ocq_E_geany_I_show_doc_com_tab
, H_ocq_E_geany_I_about_plugin
};
//------------------------------------------------------------------------------
static
void
H_ocq_E_geany_Q_action_Z_menu_X_start( GtkWidget *menu_item
, void *action_id_
){  unsigned action_id = GPOINTER_TO_UINT( action_id_ );
    if( action_id < sizeof( H_ocq_E_geany_Q_action_S ) / sizeof( *H_ocq_E_geany_Q_action_S ))
        H_ocq_E_geany_Q_action_S[action_id]();
    else if( g_mutex_trylock( &H_ocq_E_geany_Q_action_S_mutex ))
    {   _Bool result;
        if( action_id != ( unsigned )empty )
            result = H_ocq_E_compile_I_make( g_ptr_array_index( H_ocq_E_geany_Q_action_S_build_target, action_id - sizeof( H_ocq_E_geany_Q_action_S ) / sizeof( *H_ocq_E_geany_Q_action_S )));
        else
            result = H_ocq_E_compile_I_make(( void * )empty );
        if( !result )
            g_mutex_unlock( &H_ocq_E_geany_Q_action_S_mutex );
    }
}
static
gboolean
H_ocq_E_geany_Q_action_Z_keyboard_group_X_start( guint key_id
){  H_ocq_E_geany_Q_action_Z_menu_X_start( null, GUINT_TO_POINTER( key_id ));
    return yes;
}
//------------------------------------------------------------------------------
static
inline
void
H_ocq_E_geany_Q_action_Z_menu_I_add( GtkWidget *menu
, char *item_text
, char *name
, char *keybinding_text
){  GtkWidget *menu_item = gtk_menu_item_new_with_mnemonic( item_text );
    gtk_menu_shell_append(( void * )menu, menu_item );
    keybindings_set_item( H_ocq_E_geany_Q_action_Z_keyboard_group_S
	, H_ocq_E_geany_Q_action_Z_menu_I_add_S_action_id
    , null
    , 0
    , 0
    , name
    , keybinding_text
    , menu_item
    );
    g_signal_connect(( void * )menu_item, "activate", ( void * )H_ocq_E_geany_Q_action_Z_menu_X_start, GSIZE_TO_POINTER( H_ocq_E_geany_Q_action_Z_menu_I_add_S_action_id ));
	H_ocq_E_geany_Q_action_Z_menu_I_add_S_action_id++;
}
static
void
H_ocq_E_geany_Q_action_Z_menu_I_add_make( GtkWidget *menu
, char *item_text
, char *name
, char *keybinding_text
, char *target
){  g_ptr_array_add( H_ocq_E_geany_Q_action_S_build_target, target );
	H_ocq_E_geany_Q_action_Z_menu_I_add( menu, item_text, name, keybinding_text );
}
static
void
H_ocq_E_geany_Q_menu_I_add_separator( GtkWidget *menu
){  GtkWidget *menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(( void * )menu, menu_item );
}
//------------------------------------------------------------------------------
static
gboolean
E_ocq_Q_plugin_M( GeanyPlugin *plugin
, void *data
){  H_ocq_E_geany_S_plugin = plugin;
    GtkListStore *list_store = gtk_list_store_new( 4, G_TYPE_INT, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING );
    H_ocq_E_doc_com_S_page = gtk_tree_view_new_with_model(( void * )list_store );
    g_object_unref( list_store );
    gtk_tree_view_set_activate_on_single_click(( void * )H_ocq_E_doc_com_S_page, yes );
    gtk_tree_view_insert_column_with_attributes(( void * )H_ocq_E_doc_com_S_page, 0, "line", gtk_cell_renderer_text_new(), "text", 0, null );
    gtk_tree_view_insert_column_with_attributes(( void * )H_ocq_E_doc_com_S_page, 1, "doc", gtk_cell_renderer_toggle_new(), "active", 1, null );
    gtk_tree_view_insert_column_with_attributes(( void * )H_ocq_E_doc_com_S_page, 2, "tag", gtk_cell_renderer_text_new(), "text", 2, null );
    gtk_tree_view_insert_column_with_attributes(( void * )H_ocq_E_doc_com_S_page, 3, "text", gtk_cell_renderer_text_new(), "text", 3, null );
    g_signal_connect(( void * )H_ocq_E_doc_com_S_page, "row_activated", ( void * )H_ocq_E_doc_com_X_row_activated, null );
    GtkWidget *scrolled_window = gtk_scrolled_window_new( null, null );
    gtk_container_add(( void * )scrolled_window, H_ocq_E_doc_com_S_page );
    gtk_notebook_append_page(( void * )geany_data->main_widgets->message_window_notebook
    , scrolled_window
    , gtk_label_new( "doc-com" )
    );
    ui_add_document_sensitive( scrolled_window );
    gtk_widget_show( H_ocq_E_doc_com_S_page );
    gtk_widget_show( scrolled_window );
    H_ocq_E_doc_com_I_idle_update_M();
    H_ocq_E_geany_Q_action_Z_keyboard_group_S = plugin_set_key_group( H_ocq_E_geany_S_plugin
    , H_ocq_E_geany_S_plugin->info->name
    , sizeof( H_ocq_E_geany_Q_action_S ) / sizeof( *H_ocq_E_geany_Q_action_S ) + 3 + 5 + 3 + 4
    , H_ocq_E_geany_Q_action_Z_keyboard_group_X_start
    );
    GtkWidget *root_menu = gtk_menu_new();
    GtkWidget *menu;
    GtkWidget *menu_item;
    H_ocq_E_geany_Q_action_S_build_target = g_ptr_array_new();
#define H_ocq_J_gtk_menu_add_item_2(label,module,function,keybinding_function) H_ocq_E_geany_Q_action_Z_menu_I_add( menu, label, H_ocq_J_s( H_ocq_J_a_b( module, function )), H_ocq_J_s(module) ": " H_ocq_J_s( keybinding_function ))
#define H_ocq_J_gtk_menu_add_item(label,module,function) H_ocq_J_gtk_menu_add_item_2(label,module,function,function)
#define H_ocq_J_gtk_menu_add_item_make(label,target) H_ocq_E_geany_Q_action_Z_menu_I_add_make( menu, label, H_ocq_J_s( H_ocq_J_a_b( make, target )), H_ocq_J_s(make) ": " H_ocq_J_s(target), H_ocq_J_s(target) )
	H_ocq_E_geany_Q_action_Z_menu_I_add_S_action_id = 0;
    menu = gtk_menu_new();
    H_ocq_J_gtk_menu_add_item( "_open directory", project, open-directory );
    menu_item = gtk_menu_item_new_with_mnemonic( "_project" );
    gtk_menu_shell_append(( void * )root_menu, menu_item );
    gtk_menu_item_set_submenu(( void * )menu_item, menu );
	gsize action_id = H_ocq_E_geany_Q_action_Z_menu_I_add_S_action_id;
    H_ocq_E_geany_Q_action_Z_menu_I_add_S_action_id = sizeof( H_ocq_E_geany_Q_action_S ) / sizeof( *H_ocq_E_geany_Q_action_S );
    menu = gtk_menu_new();
    H_ocq_J_gtk_menu_add_item_make( "_build", build );
    H_ocq_J_gtk_menu_add_item_make( "_run", run );
    H_ocq_J_gtk_menu_add_item_make( "_install", install );
    H_ocq_E_geany_Q_menu_I_add_separator(menu);
    H_ocq_J_gtk_menu_add_item_make( "re_compile", recompile );
    H_ocq_J_gtk_menu_add_item_make( "re_doc", redoc );
    H_ocq_J_gtk_menu_add_item_make( "re_build", rebuild );
    H_ocq_J_gtk_menu_add_item_make( "rebuild & r_un", rebuild-run );
    H_ocq_J_gtk_menu_add_item_make( "re_install", reinstall );
    H_ocq_E_geany_Q_menu_I_add_separator(menu);
    H_ocq_J_gtk_menu_add_item_make( "di_st", dist );
    H_ocq_J_gtk_menu_add_item_make( "pack-_0", pack-0 );
    H_ocq_J_gtk_menu_add_item_make( "pack-_1", pack-1 );
    H_ocq_E_geany_Q_menu_I_add_separator(menu);
    H_ocq_J_gtk_menu_add_item_make( "most_lyclean", mostlyclean );
    H_ocq_J_gtk_menu_add_item_make( "clea_n", clean );
    H_ocq_J_gtk_menu_add_item_make( "distcl_ean", distclean );
    H_ocq_J_gtk_menu_add_item_make( "m_aintainer-clean", maintainer-clean );
#undef H_ocq_J_gtk_menu_add_item_make
    H_ocq_E_geany_Q_action_Z_menu_I_add_S_action_id = action_id;
    menu_item = gtk_menu_item_new_with_mnemonic( "_make" );
    gtk_menu_shell_append(( void * )root_menu, menu_item );
    gtk_menu_item_set_submenu(( void * )menu_item, menu );
    menu = gtk_menu_new();
    H_ocq_J_gtk_menu_add_item( "_html", export, html );
    menu_item = gtk_menu_item_new_with_mnemonic( "_export" );
    gtk_menu_shell_append(( void * )root_menu, menu_item );
    gtk_menu_item_set_submenu(( void * )menu_item, menu );
    menu = gtk_menu_new();
    H_ocq_J_gtk_menu_add_item( "_style", edit, style );
    menu_item = gtk_menu_item_new_with_mnemonic( "ed_it" );
    gtk_menu_shell_append(( void * )root_menu, menu_item );
    gtk_menu_item_set_submenu(( void * )menu_item, menu );
    menu = gtk_menu_new();
    H_ocq_J_gtk_menu_add_item( "_zen", snippet, zen );
    menu_item = gtk_menu_item_new_with_mnemonic( "_snippet" );
    gtk_menu_shell_append(( void * )root_menu, menu_item );
    gtk_menu_item_set_submenu(( void * )menu_item, menu );
    menu = gtk_menu_new();
    H_ocq_J_gtk_menu_add_item( "_status", show, status );
    H_ocq_J_gtk_menu_add_item( "_doc-com", show, doc-com );
    menu_item = gtk_menu_item_new_with_mnemonic( "s_how" );
    gtk_menu_shell_append(( void * )root_menu, menu_item );
    gtk_menu_item_set_submenu(( void * )menu_item, menu );
#undef H_ocq_J_gtk_menu_add_item
    H_ocq_E_geany_Q_action_S_toolbar_button = ( void * )gtk_menu_tool_button_new(
      null
    , ">"
    );
    plugin_add_toolbar_item( H_ocq_E_geany_S_plugin
    , ( void * )H_ocq_E_geany_Q_action_S_toolbar_button
    );
    gtk_menu_tool_button_set_menu(( void * )H_ocq_E_geany_Q_action_S_toolbar_button, root_menu );
    keybindings_set_item( H_ocq_E_geany_Q_action_Z_keyboard_group_S
    , H_ocq_E_geany_Q_action_Z_menu_I_add_S_action_id
    , null
    , 0
    , 0
    , "info"
    , "info"
    , H_ocq_E_geany_Q_action_S_toolbar_button
    );
    g_signal_connect(( void * )H_ocq_E_geany_Q_action_S_toolbar_button, "clicked", ( void * )H_ocq_E_geany_Q_action_Z_menu_X_start, GSIZE_TO_POINTER( H_ocq_E_geany_Q_action_Z_menu_I_add_S_action_id ));
    gtk_widget_show_all( root_menu );
    gtk_widget_show( H_ocq_E_geany_Q_action_S_toolbar_button );
    return TRUE;
}
static
void
E_ocq_Q_plugin_W( GeanyPlugin *plugin
, void *data
){  H_ocq_E_doc_com_I_idle_update_W();
    gtk_notebook_remove_page(( void * )geany_data->main_widgets->message_window_notebook
    , gtk_notebook_page_num(( void * )geany_data->main_widgets->message_window_notebook, H_ocq_E_doc_com_S_page )
    );
    gtk_widget_destroy( H_ocq_E_geany_Q_action_S_toolbar_button );
    if( H_ocq_E_compile_S_pid != empty )
    {   kill( H_ocq_E_compile_S_pid, SIGTERM );
        g_usleep(360000);
    }
    g_ptr_array_free( H_ocq_E_geany_Q_action_S_build_target, yes );
}
void
geany_load_module( GeanyPlugin *plugin
){  plugin->info->name = "Geany‐ocq utility";
    plugin->info->description =
    "• Open project directory: open files listed by file globs in the autoopen file of a directory.\n"
    "• Universal “make” runner (extended for “.cx”) with dynamic assignment of a project (filesystem directory tree) for opened source file.\n"
    "• Doc‐com: extractor of comments from a source file.\n"
    "• Export source code to ‘html’ fragment.\n"
    "• And some experimental functions.";
    plugin->info->version = "2.9";
    plugin->info->author = "Janusz Augustyński (overcq) <ocq@tutanota.com>";
    plugin->funcs->init = &E_ocq_Q_plugin_M;
    plugin->funcs->cleanup = &E_ocq_Q_plugin_W;
    GEANY_PLUGIN_REGISTER( plugin, 225 );
}
//------------------------------------------------------------------------------
PluginCallback plugin_callbacks[] =
{ { "editor_notify", ( void * )H_ocq_E_geany_X_editor_notify, yes, null }
, { "document_new", ( void * )H_ocq_E_geany_X_document_new, yes, null }
, { "document_open", ( void * )H_ocq_E_geany_X_document_open, yes, null }
, { "document_activate", ( void * )H_ocq_E_geany_X_document_activate, yes, null }
, { "document_filetype_set", ( void * )H_ocq_E_geany_X_document_filetype_set, yes, null }
, { "document_close", ( void * )H_ocq_E_geany_X_document_close, yes, null }
, { "build_start", ( void * )H_ocq_E_geany_Q_action_Z_menu_X_start, no, ( void * )empty }
, { null, null, no, null }
};
/******************************************************************************/
