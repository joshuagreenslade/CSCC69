/*
 * Copyright (c) 2004, 2008
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SIGNAL_H_
#define _SIGNAL_H_
#define SIGHUP		1
#define	SIGINT		2
#define	SIGQUIT		3
#define	SIGILL      4
#define	SIGTRAP		5
#define	SIGABRT		6
#define	SIGEMT		7
#define	SIGFPE		8	
#define	SIGKILL		9	
#define	SIGBUS		10	
#define	SIGSEGV		11	
#define	SIGSYS		12		
#define	SIGPIPE		13
#define	SIGALRM 	14	
#define	SIGTERM		15	
#define	SIGURG		16	
#define	SIGSTOP		17
#define	SIGTSTP		18
#define	SIGCONT		19
#define	SIGCHLD		20
#define	SIGTTIN		21	
#define	SIGTTOU		22
#define	SIGIO		23
#define	SIGXCPU		24
#define	SIGXFSZ		25
#define	SIGVTALRM	26	
#define	SIGPROF		27
#define	SIGWINCH	28
#define	SIGINFO		29
#define	SIGUSR1		30
#define	SIGUSR2		31	

/* In-kernel signal definitions. Get both the MD and MI parts. */

#include <kern/machine/signal.h>
#include <kern/signal.h>


#endif /* _SIGNAL_H_ */
