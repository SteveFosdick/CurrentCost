-- Script to create an empty Current Cost PostgreSQL database.

CREATE TABLE sensors (
    ix  integer PRIMARY KEY,
    label   text
);

BEGIN;
  INSERT INTO sensors(ix, label)
      VALUES (0, 'Clamp Sensor');
  INSERT INTO sensors(ix, label)
      VALUES (1, 'Computers');
  INSERT INTO sensors(ix, label)
      VALUES (2, 'TV & Audio');
  INSERT INTO sensors(ix, label)
      VALUES (3, 'Utility');
  INSERT INTO sensors(ix, label)
      VALUES (6, 'Solar (Smoothed)');
  INSERT INTO sensors(ix, label)
      VALUES (7, 'Import (Smoothed)');
  INSERT INTO sensors(ix, label)
      VALUES (8, 'Solar (Pulse)');
  INSERT INTO sensors(ix, label)
      VALUES (9, 'Import (Pulse)');
COMMIT;

CREATE TABLE power (
    time_stamp  timestamp with time zone PRIMARY KEY,
    sensor      integer,
    id          text,
    temperature real,
    watts       real,
    FOREIGN KEY (sensor)  REFERENCES sensors(ix)
);

CREATE TABLE pulse (
    time_stamp  timestamp with time zone PRIMARY KEY,
    sensor      integer,
    id          text,
    temperature real,
    pulses      integer,
    FOREIGN KEY (sensor)  REFERENCES sensors(ix)
);
