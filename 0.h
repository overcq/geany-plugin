/*******************************************************************************
*   ___   workplace
*  ¦OUX¦  ‟gtk+” condensed and ‘unix’
*  ¦Inc¦  plugin
*   ---   ‟geany” adaptation
*         base include
* ©overcq                on ‟Gentoo Linux 13.0” “x86_64”              2015‒2‒8 *
*******************************************************************************/
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <geanyplugin.h>
#include <SciLexer.h>
//==============================================================================
#define H_ocq_J_s_(s) #s
#define H_ocq_J_s(s) H_ocq_J_s_(s)
#define H_ocq_J_ab_(a,b) a##b
#define H_ocq_J_ab(a,b) H_ocq_J_ab_( a, b )
#define H_ocq_J_a_b(a,b) H_ocq_J_ab( H_ocq_J_ab( a, _ ), b )
//------------------------------------------------------------------------------
#define no false
#define yes true
#define null NULL
#define empty (~0UL)
#define O while(empty)
//==============================================================================
#define MSG_DOC_COM ( MSG_VTE + 1 )
/******************************************************************************/
