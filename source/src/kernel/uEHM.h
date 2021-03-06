//                              -*- Mode: C++ -*- 
// 
// uC++ Version 6.1.0, Copyright (C) Russell Mok 1997
// 
// uEHM.h -- 
// 
// Author           : Russell Mok
// Created On       : Mon Jun 30 16:46:18 1997
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu May 26 16:14:58 2016
// Update Count     : 490
//
// This  library is free  software; you  can redistribute  it and/or  modify it
// under the terms of the GNU Lesser General Public License as published by the
// Free Software  Foundation; either  version 2.1 of  the License, or  (at your
// option) any later version.
// 
// This library is distributed in the  hope that it will be useful, but WITHOUT
// ANY  WARRANTY;  without even  the  implied  warranty  of MERCHANTABILITY  or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
// for more details.
// 
// You should  have received a  copy of the  GNU Lesser General  Public License
// along  with this library.
// 


#ifndef __U_EHM_H__
#define __U_EHM_H__

#include <typeinfo>
#include <functional>

#define uRendezvousAcceptor uSerialMemberInstance.uAcceptor
#define uEHMMaxMsg 156
#define uEHMMaxName 100

class uEHM;						// forward declaration


//######################### uBaseEvent ########################


class uBaseEvent {
    friend class uEHM;
  public:
    enum RaiseKind { ThrowRaise, ResumeRaise };
  protected:
    const uBaseCoroutine *src;				// source execution for async raise, set at raise
    char srcName[uEHMMaxName];				//    and this field, too
    char msg[uEHMMaxMsg];				// message to print if exception uncaught
    mutable void *staticallyBoundObject;		// bound object for matching, set at raise
    mutable RaiseKind raiseKind;			// how the exception is raised

    uBaseEvent( const char *const msg = "" ) { src = NULL; setMsg( msg ); }
    void setSrc( uBaseCoroutine &coroutine );
    const std::type_info *getEventType() const { return &typeid( *this ); };
    void setMsg( const char *const msg );
    virtual void stackThrow() const __attribute__(( noreturn )) = 0; // translator generated => object specific
  public:
    virtual ~uBaseEvent();

    const char *message() const { return msg; }
    const uBaseCoroutine &source() const { return *src; }
    const char *sourceName() const { return src != NULL ? srcName : "*unknown*"; }
    RaiseKind getRaiseKind() const { return raiseKind; }
    const void *getOriginalThrower() const { return staticallyBoundObject; }
    void reraise();
    virtual uBaseEvent *duplicate() const = 0;		// translator generated => object specific
    virtual void defaultTerminate();
    virtual void defaultResume();
}; // uBaseEvent


//######################### uEHM ########################


class uEHM {
    friend class UPP::uKernelBoot;			// access: terminateHandler, unexpectedHandler
    friend class UPP::uMachContext;			// access: terminate
    friend class uBaseCoroutine;			// access: ResumeWorkHorseInit, uResumptionHandlers, uDeliverEStack, unexpected, strncpy
    friend class uBaseTask;				// access: terminateHandler
    friend class uBaseEvent;				// access: AsyncEMsg

    class ResumeWorkHorseInit;
    class AsyncEMsg;
    class AsyncEMsgBuffer;

    static bool match_exception_type( const std::type_info *derived_type, const std::type_info *parent_type );
    static bool deliverable_exception( const std::type_info *event_type );
    static void terminate() __attribute__(( noreturn ));
    static void terminateHandler() __attribute__(( noreturn ));
    static void unexpected() __attribute__(( noreturn ));
    static void unexpectedHandler() __attribute__(( noreturn ));
  public:
    enum RETURN_DISALLOWED { NORETURN };		// prevent returns in lambda body

    class uResumptionHandlers;				// usage generated by translator
    template< typename Functor > class uRoutineHandlerAny;
    template< typename Exn, typename Functor > class uRoutineHandler;
    class uFinallyHandler;

    class uHandlerBase;
    class uDeliverEStack;

    static void asyncToss( const uBaseEvent &event, uBaseCoroutine &target, uBaseEvent::RaiseKind raiseKind, bool rethrow = false );
    static void asyncReToss( uBaseCoroutine &target, uBaseEvent::RaiseKind raiseKind );

    static void Throw( const uBaseEvent &event, void *const bound = 0 ) __attribute__(( noreturn ));
    static void Throw( const uBaseEvent &event, uBaseCoroutine &target ) { asyncToss( event, target, uBaseEvent::ThrowRaise ); }
    static void Throw( uBaseCoroutine &target ) { asyncReToss( target, uBaseEvent::ThrowRaise ); } // asynchronous rethrow
    static void ReThrow() __attribute__(( noreturn ));	// synchronous rethrow

    static void Resume( const uBaseEvent &event, void *const bound = 0 );
    static void Resume( const uBaseEvent &event, uBaseCoroutine &target ) { asyncToss( event, target, uBaseEvent::ResumeRaise ); }
    static void Resume( uBaseCoroutine &target ) { asyncReToss( target, uBaseEvent::ResumeRaise ); } // asynchronous reresume
    static void ReResume();

    static bool pollCheck();
    static int poll();
    static const std::type_info *getTopResumptionType();
    static uBaseEvent *getCurrentException();
    static uBaseEvent *getCurrentResumption();
    static char *getCurrentEventName( uBaseEvent::RaiseKind raiseKind, char *s1, size_t n );
    static char *strncpy( char *s1, const char *s2, size_t n );
  private:
    static void resumeWorkHorse( const uBaseEvent &event, bool conseq );
}; // uEHM


//######################### uEHM::AsyncEMsg ########################


class uEHM::AsyncEMsg : public uSeqable {
    friend class uEHM;
    friend class uEHM::AsyncEMsgBuffer;
    friend void uEHM::Throw( const uBaseEvent &, uBaseCoroutine & );
    friend void uEHM::Resume( const uBaseEvent &, uBaseCoroutine & );

    bool hidden;
    uBaseEvent *asyncEvent;

    AsyncEMsg &operator=( const AsyncEMsg & );
    AsyncEMsg( const AsyncEMsg & );

    AsyncEMsg( const uBaseEvent &event );
  public:
    ~AsyncEMsg();
}; // uEHM::AsyncEMsg


//######################### uEHM::AsyncEMsgBuffer ########################


// AsyncEMsgBuffer looks like public uQueue<AsyncEMsg> but with mutex

class uEHM::AsyncEMsgBuffer : public uSequence<uEHM::AsyncEMsg> {
    friend class UPP::uTaskMain;

    AsyncEMsgBuffer( const AsyncEMsgBuffer & );
    AsyncEMsgBuffer& operator=( const AsyncEMsgBuffer & );
  public:
    uSpinLock lock;
    AsyncEMsgBuffer();
    ~AsyncEMsgBuffer();
    void uAddMsg( AsyncEMsg *msg );
    AsyncEMsg *uRmMsg();
    AsyncEMsg *uRmMsg( AsyncEMsg *msg );
    AsyncEMsg *nextVisible( AsyncEMsg *msg );
}; // uEHM::AsyncEMsgBuffer


//######################### internal class and function declarations ########################


// base class allowing a list of otherwise-heterogeneous uHandlers
class uEHM::uHandlerBase {
    const void *const matchBinding;
    const std::type_info *eventType;
  protected:
    uHandlerBase( const void *matchBinding, const std::type_info *eventType ) : matchBinding( matchBinding ), eventType( eventType ) {}
    virtual ~uHandlerBase() {}
  public:
    virtual void uHandler( uBaseEvent &exn ) = 0;
    const void *getMatchBinding() const { return matchBinding; }
    const std::type_info *getEventType() const { return eventType; }
}; // uHandlerBase

template< typename Exn >
class uRoutineHandler : public uEHM::uHandlerBase {
    const std::function< uEHM::RETURN_DISALLOWED ( Exn & ) > handlerRtn; // lambda for exception handling routine
  public:
    uRoutineHandler( const std::function< uEHM::RETURN_DISALLOWED ( Exn & ) > &handlerRtn ) : uHandlerBase( 0, &typeid( Exn ) ), handlerRtn( handlerRtn ) {}
    uRoutineHandler( const void *originalThrower, const std::function< uEHM::RETURN_DISALLOWED ( Exn & ) > &handlerRtn ) : uHandlerBase( originalThrower, &typeid( Exn ) ), handlerRtn( handlerRtn ) {}
    virtual void uHandler( uBaseEvent &exn ) { handlerRtn( (Exn &)exn ); }
}; // uRoutineHandler

class uRoutineHandlerAny : public uEHM::uHandlerBase {
    const std::function< uEHM::RETURN_DISALLOWED () > handlerRtn; // lambda for exception handling routine
  public:
    uRoutineHandlerAny( const std::function< uEHM::RETURN_DISALLOWED () > &handlerRtn ) : uHandlerBase( 0, 0 ), handlerRtn( handlerRtn ) {}
    virtual void uHandler( uBaseEvent &exn ) { handlerRtn(); }
}; // uRoutineHandlerAny


// Every set of resuming handlers bound to a template try block is saved in a uEHM::uResumptionHandlers object. The
// resuming handler hierarchy is implemented as a linked list.

class uEHM::uResumptionHandlers {
    friend void uEHM::resumeWorkHorse( const uBaseEvent &, bool );

    uResumptionHandlers *next, *conseqNext;		// uNext maintains a proper stack, while uConseqNext is used to skip
							// over handlers that have already been examined for resumption (to avoid recursion)

    const unsigned int size;				// number of handlers
    uHandlerBase *const *table;				// pointer to array of resumption handlers

    uResumptionHandlers( const uResumptionHandlers & );	// no copy
    uResumptionHandlers &operator=( const uResumptionHandlers & ); // no assignment
  public:
    uResumptionHandlers( uHandlerBase *const table[], const unsigned int size );
    ~uResumptionHandlers();
}; // uEHM::uResumptionHandlers


// The following implements a linked list of event_id's table.  Used in enable and disable block.

class uEHM::uDeliverEStack {
    friend bool uEHM::deliverable_exception( const std::type_info * );

    uDeliverEStack *next;
    bool deliverFlag;					// true when events in table is Enable, otherwise false
    int  table_size;                                    // number of events in the table, 0 implies everything
    const std::type_info **event_table;			// event id table

    uDeliverEStack( uDeliverEStack & );			// no copy
    uDeliverEStack &operator=( uDeliverEStack & );	// no assignment
  public:
    uDeliverEStack( bool f, const std::type_info **t = NULL, unsigned int msg = 0 ); // for enable and disable blocks
    ~uDeliverEStack();
}; // uEHM::uDeliverEStack


// Finally block is hoisted to lambda and invoked by RAII object nested in block surrounding "try" statement.

class uEHM::uFinallyHandler {
    const std::function< RETURN_DISALLOWED () > cleanUpRtn; // lambda for clean up
  public:
    uFinallyHandler( const std::function< RETURN_DISALLOWED () > &cleanUpRtn ) : cleanUpRtn( cleanUpRtn ) {}
    ~uFinallyHandler()
#if __cplusplus >= 201103L
	    noexcept( false )				// C++11, required to allow exception from destructor
#endif
    {
	cleanUpRtn();					// invoke handler on block exit
    } // uFinallyHandler::~uFinallyHandler
}; // uEHM::uFinallyHandler
#endif // __U_EHM_H__


// Local Variables: //
// compile-command: "make install" //
// End: //
