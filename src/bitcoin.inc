<?php

define("BITCOIN_ADDRESS_VERSION", "00");// this is a hex byte
/**
 * Bitcoin utility functions class
 * @author theymos (functionality)
 * @author Mike Gogulski (encapsulation, string abstraction, PHPDoc)
 */
class Bitcoin {

  /*
   * Bitcoin utility functions by theymos
   * Via http://www.bitcoin.org/smf/index.php?topic=1844.0
   * hex input must be in uppercase, with no leading 0x
   */
  private static $hexchars = "0123456789ABCDEF";
  private static $base58chars = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

  /**
   * Convert a hex string into a (big) integer
   * @param string $hex
   * @return int
   * @access private
   */
  private function decodeHex($hex) {
    $hex = strtoupper($hex);
    $return = "0";
    for ($i = 0; $i < strlen($hex); $i++) {
      $current = (string) strpos(self::$hexchars, $hex[$i]);
      $return = (string) bcmul($return, "16", 0);
      $return = (string) bcadd($return, $current, 0);
    }
    return $return;
  }

  /**
   * Convert an integer into a hex string
   * @param int $dec
   * @return string
   * @access private
   */
  private function encodeHex($dec) {
    $return = "";
    while (bccomp($dec, 0) == 1) {
      $dv = (string) bcdiv($dec, "16", 0);
      $rem = (integer) bcmod($dec, "16");
      $dec = $dv;
      $return = $return . self::$hexchars[$rem];
    }
    return strrev($return);
  }

  /**
   * Convert a Base58-encoded integer into the equivalent hex string representation
   * @param string $base58
   * @return string
   * @access private
   */
  private function decodeBase58($base58) {
    $origbase58 = $base58;

    $return = "0";
    for ($i = 0; $i < strlen($base58); $i++) {
      $current = (string) strpos(Bitcoin::$base58chars, $base58[$i]);
      $return = (string) bcmul($return, "58", 0);
      $return = (string) bcadd($return, $current, 0);
    }

    $return = self::encodeHex($return);

    //leading zeros
    for ($i = 0; $i < strlen($origbase58) && $origbase58[$i] == "1"; $i++) {
      $return = "00" . $return;
    }

    if (strlen($return) % 2 != 0) {
      $return = "0" . $return;
    }

    return $return;
  }

  /**
   * Convert a hex string representation of an integer into the equivalent Base58 representation
   * @param string $hex
   * @return string
   * @access private
   */
  private function encodeBase58($hex) {
    if (strlen($hex) % 2 != 0) {
      die("encodeBase58: uneven number of hex characters");
    }
    $orighex = $hex;

    $hex = self::decodeHex($hex);
    $return = "";
    while (bccomp($hex, 0) == 1) {
      $dv = (string) bcdiv($hex, "58", 0);
      $rem = (integer) bcmod($hex, "58");
      $hex = $dv;
      $return = $return . self::$base58chars[$rem];
    }
    $return = strrev($return);

    //leading zeros
    for ($i = 0; $i < strlen($orighex) && substr($orighex, $i, 2) == "00"; $i += 2) {
      $return = "1" . $return;
    }

    return $return;
  }

  /**
   * Convert a 160-bit Bitcoin hash to a Bitcoin address
   * @author theymos
   * @param string $hash160
   * @param string $addressversion
   * @return string Bitcoin address
   * @access public
   */
  public static function hash160ToAddress($hash160, $addressversion = BITCOIN_ADDRESS_VERSION) {
    $hash160 = $addressversion . $hash160;
    $check = pack("H*", $hash160);
    $check = hash("sha256", hash("sha256", $check, true));
    $check = substr($check, 0, 8);
    $hash160 = strtoupper($hash160 . $check);
    return self::encodeBase58($hash160);
  }

  /**
   * Convert a Bitcoin address to a 160-bit Bitcoin hash
   * @author theymos
   * @param string $addr
   * @return string Bitcoin hash
   * @access public
   */
  public static function addressToHash160($addr) {
    $addr = self::decodeBase58($addr);
    $addr = substr($addr, 2, strlen($addr) - 10);
    return $addr;
  }

  /**
   * Determine if a string is a valid Bitcoin address
   * @author theymos
   * @param string $addr String to test
   * @param string $addressversion
   * @return boolean
   * @access public
   */
  public static function checkAddress($addr, $addressversion = BITCOIN_ADDRESS_VERSION) {
    $addr = self::decodeBase58($addr);
    if (strlen($addr) != 50) {
      return false;
    }
    $version = substr($addr, 0, 2);
    if (hexdec($version) > hexdec($addressversion)) {
      return false;
    }
    $check = substr($addr, 0, strlen($addr) - 8);
    $check = pack("H*", $check);
    $check = strtoupper(hash("sha256", hash("sha256", $check, true)));
    $check = substr($check, 0, 8);
    return $check == substr($addr, strlen($addr) - 8);
  }

  /**
   * Convert the input to its 160-bit Bitcoin hash
   * @param string $data
   * @return string
   * @access private
   */
  private function hash160($data) {
    $data = pack("H*", $data);
    return strtoupper(hash("ripemd160", hash("sha256", $data, true)));
  }

  /**
   * Convert a Bitcoin public key to a 160-bit Bitcoin hash
   * @param string $pubkey
   * @return string
   * @access public
   */
  public static function pubKeyToAddress($pubkey) {
    return self::hash160ToAddress($this->hash160($pubkey));
  }

  /**
   * Remove leading "0x" from a hex value if present.
   * @param string $string
   * @return string
   * @access public
   */
  public static function remove0x($string) {
    if (substr($string, 0, 2) == "0x" || substr($string, 0, 2) == "0X") {
      $string = substr($string, 2);
    }
    return $string;
  }
}

class BitcoinClientException extends ErrorException {
  // Redefine the exception so message and severity aren't optional
  public function __construct($message, $code = 0, $severity = E_USER_NOTICE, Exception $previous = null) {
    parent::__construct($message, $code, $severity, $previous);
  }

  public function __toString() {
    return __CLASS__ . ": [{$this->code}]: {$this->message}\n";
  }
}

require_once(dirname(__FILE__) . "/includes/xmlrpc.inc");
require_once(dirname(__FILE__) . "/includes/jsonrpc.inc");

/**
 * Bitcoin client class for access to a Bitcoin server via JSON-RPC-HTTP[S]
 * @author Mike Gogulski
 */
class BitcoinClient extends jsonrpc_client {

  /**
   * Create a jsonrpc_client object to talk to the bitcoin server and return it, or false on failure.
   * @param string $scheme "http" or "https"
   * @param string $username User name to use in connection the Bitcoin server's JSON-RPC interface
   * @param string $password Server password
   * @param string $address Server hostname or IP address
   * @param mixed $port Server port (string or integer)
   * @param string $certificate_path Path on the local filesystem to server's PEM certificate (ignored if $scheme != "https")
   * @param integer $debug_level 0 (default) = no debugging; 1 = echo JSON-RPC messages received to stdout; 2 = log transmitted messages also
   * @return jsonrpc_client
   * @access public
   * @throws BitcoinClientException
   */
  public function __construct($scheme, $username, $password, $address = "localhost", $port = 8332, $certificate_path = '', $debug_level = 0) {
    $scheme = strtolower($scheme);
    if ($scheme != "http" && $scheme != "https")
      throw new BitcoinClientException("Scheme must be http or https");
    if (empty($username))
      throw new BitcoinClientException("Username must be non-blank");
    if (empty($password))
      throw new BitcoinClientException("Password must be non-blank");
    $port = (string) $port;
    if (empty($port) || !is_numeric($port) || $port < 0 || $port > 65535 || floatval($port) != intval($port))
      throw new BitcoinClientException("Port must be an integer and between 0 and 65535");
    if (!empty($certificate_path) && !is_readable($certificate_path))
      throw new BitcoinClientException("Certificate file " . $certificate_path . " is not readable");
    $uri = $scheme . "://" . $username . ":" . $password . "@" . $address . ":" . $port . "/";
    parent::__construct($uri);
    $this->setDebug($debug_level);
    $this->setSSLVerifyHost(0);
    if ($scheme == "https")
      if (!empty($certificate_path))
        $this->setCaCertificate($certificate_path);
      else
        $this->setSSLVerifyPeer(false);
  }

  /**
   * Test if the connection to the Bitcoin JSON-RPC server is working
   *
   * The check is done by calling the server's getinfo() method and checking for a fault.
   * @return mixed boolean TRUE if successful, cURL fault string otherwise
   * @access public
   * @throws none
   */
  public function can_connect() {
    try {
      $r = $this->query("getinfo");
    } catch (BitcoinClientException $e) {
      return $e->getMessage();
    }
    return true;
  }

  /**
   * Convert a Bitcoin server query argument to a jsonrpcval
   * @param mixed $a
   * @return jsonrpcval
   * @throws none
   */
  private function query_arg_to_parameter($a) {
    $type = "";// "string" is encoded as this default type value in xmlrpc.inc
    if (is_numeric($a)) {
      if (intval($a) != floatval($a)) {
        $a = intval($a);
        $type = "int";
      } else {
        $a = floatval($a);
        $type = "double";
      }
    }
    if (is_bool($a))
      $type = "boolean";
    if (is_int($a))
      $type = "int";
    if (is_float($a))
      $type = "double";
    if (is_array($a))
      $type = "array";
    return new jsonrpcval($a, $type);
  }

  /**
   * Send a JSON-RPC message and optional parameter arguments to the server
   * @param string $message
   * @param mixed $args,...
   * @return mixed
   * @throws BitcoinClientException
   * @see xmlrpc.inc:php_xmlrpc_decode()
   */
  public function query($message) {
    if (!$message || empty($message))
      throw new BitcoinClientException("Client query requires a message");
    $msg = new jsonrpcmsg($message);
    if (func_num_args() > 1) {
      array_shift($func_get_args());
      foreach ($func_get_args() as $a) {
        $msg->addParam(query_arg_to_parameter($a));
      }
    }
    $response = $this->send($msg);
    if ($response->faultCode()) {
      throw new BitcoinClientException($response->faultString());
    }
    return php_xmlrpc_decode($response->value());
  }
}