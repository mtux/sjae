/*
Unified application and message log style sheet
*/

/* some things to try */
/* SplitterWin > QTextEdit#edMsgLog { background-color: gray } */
/* QTreeWidget { background-color: transparent } */
/* MessageWin > QTextEdit#edMsg { color: blue } */

/* resize all text edit control text */
/*
QTextEdit {
  font-size: 16px;
  font-family: Tahoma
}
*/

/* message log items - other span classes include timestamp, date, and time */
span.incomming > span.message { color: red }
span.outgoing { color: gray }
span.outgoing > span.message { color: blue }

