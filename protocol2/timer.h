/* DO NOT EDIT THIS FILE. It was created by extractDecls */
/*
 * A countdown timer.
 *
 * See ../COPYRIGHT file for copying and redistribution conditions.
 */
#ifndef TIMER_H
#define TIMER_H

typedef struct timer    Timer;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Starts a countdown timer.
 *
 * Arguments:
 *      seconds         The coundown time-interval in seconds.
 * Returns:
 *      NULL            Error. "log_add()" called.
 *      else            Pointer to the timer data-structure.
 */
Timer*
timer_new(
    const unsigned long seconds);

/*
 * Frees a timer.
 *
 * Arguments
 *      timer           Pointer to the timer data-structure to be freed or
 *                      NULL.
 */
void
timer_free(
    Timer* const        timer);

/*
 * Indicates if a timer has elapsed.
 *
 * Arguments:
 *      timer           The timer data-structure.
 * Returns:
 *      0               The timer has not elapsed.
 *      else            The timer has elapsed.
 */
int
timer_hasElapsed(
    const Timer* const  timer);

#ifdef __cplusplus
}
#endif

#endif
