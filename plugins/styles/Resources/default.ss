/*
Unified application and message log style sheet
*/

/* some things to try */

/* SplitterWin > #edMsgLog { background-color: gray } */
/* 
QTreeWidget { 
  background-color: transparent;
  qproperty-indentation: 10
} 
*/
/* MessageWin > #edMsg { color: blue } */

/* resize all text edit control text */
/*
QTextEdit {
  font-size: 16px;
  font-family: Tahoma
}
*/

/* message log items

div classes are:
  incomming
    message
  outgoing
    message
  chat_state

span classes (in message div) are:
  info
    timestamp
      date or date_today
      time
    nick
    separator
  text

span classes (in chat_state div) are:
  nick
  state_text
  
*/

.date_today { display: none }
/* div.message > span.nick { display: none } */

div.chat_state { color: lightgray }
div.outgoing { color: gray }

div.incomming > div.message > span.text { color: red }
div.outgoing > div.message > span.text { color: blue }


/* group blocks */

span.info { display: none }

div.message { margin-left: 20px}

div.incomming > div.message:first-child > span.info {   display: inline }
div.incomming > div.message:first-child { margin-left: 0px }
div.outgoing > div.message:first-child > span.info { display: inline }
div.outgoing > div.message:first-child { margin-left: 0px }