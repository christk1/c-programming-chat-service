insert into groups (GroupName) values ('group4');
select * from groups;
insert into users (UName,UPass) values('George','password');
select * from users;
insert into users_has_groups(userIDbelongs,TOgroupID) values (5,1);
select * from users_has_groups order by TOgroupID;
insert into PrivateMessages(senderID,receiver,message) values (5,1,"another good message");
select * from PrivateMessages;
insert into GroupMessages(sender,senderGroup,message) values (5,2,"Hello");
select * from GroupMessages;