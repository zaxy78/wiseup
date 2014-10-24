-- phpMyAdmin SQL Dump
-- version 3.4.11.1deb2+deb7u1
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: Oct 20, 2014 at 07:27 PM
-- Server version: 5.5.38
-- PHP Version: 5.4.4-14+deb7u12

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `wiseup`
--
CREATE DATABASE `wiseup` DEFAULT CHARACTER SET latin1 COLLATE latin1_swedish_ci;
USE `wiseup`;

DELIMITER $$
--
-- Procedures
--
CREATE DEFINER=`root`@`localhost` PROCEDURE `sp_get_sensors_info`(
sensor_hub_address_t bigint )
BEGIN

SELECT SI.sensor_id, SI.sensor_name, SI.sensor_family_type, SI.available, SD.time_stamp, SD.value, SA.action
FROM  `sensor-info` SI  
JOIN `sensor-data` SD ON SI.sensor_id = SD.sensor_id
LEFT JOIN `sensor-action` SA ON SI.sensor_id = SA.sensor_id;

END$$

CREATE DEFINER=`root`@`localhost` PROCEDURE `sp_set_sensors_action`(
sensor_id_t bigint,
sensor_action int )
BEGIN

UPDATE `wiseup`.`sensor-action`
SET `action` = sensor_action
WHERE sensor_id = sensor_id_t;

SELECT SI.sensor_hub_address, SI.sensor_address, SA.action 
FROM `wiseup`.`sensor-action` SA 
JOIN `wiseup`.`sensor-info` SI ON SI.sensor_id = SA.sensor_id
WHERE SA.sensor_id = sensor_id_t;

END$$

CREATE DEFINER=`root`@`localhost` PROCEDURE `sp_update_sensor_info`(
sensor_id_t bigint,
sensor_hub_address_t bigint,
sensor_address_t tinyint,
sensor_family_type_t tinyint,
available_t tinyint,
value_t int)
BEGIN
	IF EXISTS (SELECT * FROM  `sensor-info`  WHERE `sensor_id` = sensor_id_t) THEN
BEGIN
	UPDATE `sensor-data`
	SET `time_stamp` = UNIX_TIMESTAMP(),`value` = value_t
	WHERE `sensor_id` = sensor_id_t;
INSERT INTO `wiseup`.`sensor-data-history`(`record_id`, `sensor_id`, `time_stamp`, `value`) VALUES (NULL,sensor_id_t,UNIX_TIMESTAMP(),value_t);
END;
ELSE
BEGIN

	INSERT INTO `wiseup`.`sensor-info` (`sensor_id`, `sensor_hub_address`, `sensor_address`, `sensor_name`, `sensor_family_type`, `registration_datetime`, `update_interval`, `available`) VALUES (sensor_id_t,sensor_hub_address_t,sensor_address_t,'New sensor',sensor_family_type_t,NOW(),'60',available_t);

	INSERT INTO `wiseup`.`sensor-data` (`sensor_id`, `time_stamp`, `value`) VALUES (sensor_id_t,UNIX_TIMESTAMP(),value_t);

IF sensor_family_type_t = 4 THEN
BEGIN
INSERT INTO `wiseup`.`sensor-action` (`sensor_id`, `action`) VALUES (sensor_id_t, 0);
END;
END IF;

END;
END IF;
END$$

DELIMITER ;

-- --------------------------------------------------------

--
-- Table structure for table `sensor-action`
--

CREATE TABLE IF NOT EXISTS `sensor-action` (
  `sensor_id` bigint(20) unsigned NOT NULL,
  `action` int(11) NOT NULL,
  PRIMARY KEY (`sensor_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `sensor-data`
--

CREATE TABLE IF NOT EXISTS `sensor-data` (
  `sensor_id` bigint(20) unsigned NOT NULL,
  `time_stamp` bigint(20) unsigned NOT NULL,
  `value` int(11) NOT NULL,
  PRIMARY KEY (`sensor_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `sensor-data-history`
--

CREATE TABLE IF NOT EXISTS `sensor-data-history` (
  `record_id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `sensor_id` bigint(20) unsigned NOT NULL,
  `time_stamp` bigint(20) NOT NULL,
  `value` int(11) NOT NULL,
  PRIMARY KEY (`record_id`)
) ENGINE=InnoDB  DEFAULT CHARSET=latin1 AUTO_INCREMENT=120697 ;

-- --------------------------------------------------------

--
-- Table structure for table `sensor-info`
--

CREATE TABLE IF NOT EXISTS `sensor-info` (
  `sensor_id` bigint(20) unsigned NOT NULL,
  `sensor_hub_address` bigint(20) unsigned NOT NULL,
  `sensor_address` tinyint(3) unsigned NOT NULL,
  `sensor_name` varchar(128) NOT NULL,
  `sensor_family_type` tinyint(3) unsigned NOT NULL,
  `registration_datetime` datetime NOT NULL,
  `update_interval` smallint(5) unsigned NOT NULL,
  `available` tinyint(1) unsigned NOT NULL,
  PRIMARY KEY (`sensor_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;