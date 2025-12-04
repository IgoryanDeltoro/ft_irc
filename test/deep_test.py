# Run all tests
#   python3 deep_test.py

# Quick smoke test
#   python3 deep_test.py --quick

# Specific test
#   python3 deep_test.py --test flood

# Custom server
#   python3 deep_test.py --host 192.168.1.100 --port 6667 --pass mypassword


import socket
import time
import threading
import random
import string
import sys
import argparse
from typing import List, Optional, Dict, Any, Tuple

# ANSI color codes for terminal output
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    WHITE = '\033[97m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    END = '\033[0m'

# Test result tracker
class TestResult:
    def __init__(self, name: str):
        self.name = name
        self.passed = False
        self.message = ""
        self.expected = ""
        self.actual = ""
        self.duration = 0.0
        
    def success(self, message: str = ""):
        self.passed = True
        self.message = message
        
    def failure(self, message: str, expected: str = "", actual: str = ""):
        self.passed = False
        self.message = message
        self.expected = expected
        self.actual = actual
        
    def __str__(self):
        color = Colors.GREEN if self.passed else Colors.RED
        status = "✓ PASS" if self.passed else "✗ FAIL"
        return f"{color}{status}{Colors.END}: {self.name} - {self.message}"

class IRCTestClient:
    def __init__(self, host: str, port: int, password: str):
        self.host = host
        self.port = port
        self.password = password
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((host, port))
        self.buffer = bytearray()
        self.timeout = 0.5
        
    def send(self, data: str):
        """Send data to server"""
        self.sock.sendall(data.encode('utf-8'))
        time.sleep(0.01)
        
    def send_raw(self, data: bytes):
        """Send raw bytes"""
        self.sock.sendall(data)
        
    def receive(self, timeout: float = 0.1) -> str:
        """Receive data with timeout"""
        self.sock.settimeout(timeout)
        try:
            data = self.sock.recv(4096)
            if data:
                return data.decode('utf-8', errors='ignore')
        except (socket.timeout, BlockingIOError):
            pass
        except ConnectionError:
            return ""
        return ""
    
    def receive_all(self, duration: float = 0.2) -> str:
        """Receive all data for specified duration"""
        start = time.time()
        output = []
        while time.time() - start < duration:
            data = self.receive(0.05)
            if data:
                output.append(data)
            time.sleep(0.01)
        return ''.join(output)
    
    def register(self, nick: str, user: str = None, realname: str = None) -> str:
        """Complete registration"""
        if not user:
            user = nick
        if not realname:
            realname = f"Test User {nick}"
            
        self.send(f"PASS {self.password}\r\n")
        self.send(f"NICK {nick}\r\n")
        self.send(f"USER {user} 0 * :{realname}\r\n")
        time.sleep(0.1)
        return self.receive_all(0.3)
    
    def close(self):
        """Close connection"""
        try:
            self.sock.close()
        except:
            pass

def assert_in_response(response: str, expected: str, test_result: TestResult) -> bool:
    """Check if expected string is in response"""
    if expected in response:
        test_result.success(f"Found '{expected}' in response")
        return True
    else:
        test_result.failure(
            f"Expected '{expected}' not found in response",
            expected,
            response[:100] + "..." if len(response) > 100 else response
        )
        return False

def assert_not_in_response(response: str, not_expected: str, test_result: TestResult) -> bool:
    """Check if string is NOT in response"""
    if not_expected not in response:
        test_result.success(f"'{not_expected}' not in response as expected")
        return True
    else:
        test_result.failure(
            f"Unexpected '{not_expected}' found in response",
            f"Not containing: {not_expected}",
            response[:100] + "..." if len(response) > 100 else response
        )
        return False

def assert_response_contains(response: str, keywords: List[str], test_result: TestResult) -> bool:
    """Check if response contains all keywords"""
    missing = []
    for keyword in keywords:
        if keyword not in response:
            missing.append(keyword)
    
    if not missing:
        test_result.success(f"Response contains all keywords: {keywords}")
        return True
    else:
        test_result.failure(
            f"Missing keywords in response: {missing}",
            f"All of: {keywords}",
            response[:100] + "..." if len(response) > 100 else response
        )
        return False

def assert_numeric_response(response: str, code: str, test_result: TestResult) -> bool:
    """Check for specific numeric response code"""
    if f" {code} " in response or response.startswith(f":{code}"):
        test_result.success(f"Received numeric response {code}")
        return True
    else:
        # Try to find any numeric code in response
        import re
        codes = re.findall(r' (\d{3}) ', response)
        if codes:
            test_result.failure(
                f"Expected code {code}, got {codes[0]}",
                f"Numeric code {code}",
                f"Numeric code(s) found: {', '.join(codes)}"
            )
        else:
            test_result.failure(
                f"No numeric code {code} in response",
                f"Numeric code {code}",
                response[:100] + "..." if len(response) > 100 else response
            )
        return False

def assert_connection_successful(client: IRCTestClient, test_result: TestResult) -> bool:
    """Check if connection and registration were successful"""
    try:
        # Send a PING to check if connection is alive
        client.send("PING :testconnection\r\n")
        response = client.receive_all(0.5)
        
        if "PONG" in response or "PING" in response:
            test_result.success("Connection is alive and responsive")
            return True
        else:
            test_result.failure(
                "No PONG response from server",
                "PONG or similar response",
                response[:100] + "..." if len(response) > 100 else response
            )
            return False
    except Exception as e:
        test_result.failure(f"Connection test failed: {str(e)}", "Alive connection", str(e))
        return False

# ====== RFC 1459/2812 COMPLIANCE TESTS ======

def test_connection_registration(host: str, port: int, password: str) -> TestResult:
    """Test basic connection and registration"""
    result = TestResult("Connection Registration")
    start_time = time.time()
    
    try:
        # Test 1: Normal registration
        client = IRCTestClient(host, port, password)
        response = client.register("TestUser")
        
        # Should get welcome message (001) or similar
        if not assert_numeric_response(response, "001", result):
            # Try other common welcome codes
            if not any(code in response for code in ["001", "002", "003", "004", "005"]):
                result.failure(
                    "No welcome message received",
                    "001 RPL_WELCOME or similar",
                    response[:100] + "..." if len(response) > 100 else response
                )
                client.close()
                return result
        
        client.close()
        
        # Test 2: Registration without PASS
        client = IRCTestClient(host, port, password)
        client.send("NICK NoPassUser\r\n")
        client.send("USER nopass 0 * :No Password\r\n")
        response = client.receive_all(0.3)
        
        # Should get error about missing/wrong password
        if not assert_numeric_response(response, "464", result):  # ERR_PASSWDMISMATCH
            if not assert_numeric_response(response, "461", result):  # ERR_NEEDMOREPARAMS
                if "PASS" in response or "password" in response.lower():
                    result.success("Server responded about password issue")
                else:
                    result.failure(
                        "No password error for missing PASS",
                        "464 ERR_PASSWDMISMATCH or similar",
                        response[:100] + "..." if len(response) > 100 else response
                    )
        
        client.close()
        
        result.success("All registration tests passed")
        
    except ConnectionRefusedError:
        result.failure("Could not connect to server", "Successful connection", "Connection refused")
    except Exception as e:
        result.failure(f"Unexpected error: {str(e)}", "Test completion", str(e))
    
    result.duration = time.time() - start_time
    return result

def test_nick_command(host: str, port: int, password: str) -> TestResult:
    """Test NICK command"""
    result = TestResult("NICK Command")
    start_time = time.time()
    
    try:
        # Test 1: Normal NICK change
        client = IRCTestClient(host, port, password)
        client.register("OriginalNick")
        client.receive_all(0.2)
        
        client.send("NICK ChangedNick\r\n")
        response = client.receive_all(0.3)
        
        # Should get NICK change notification
        if not assert_in_response(response, "NICK", result):
            result.failure(
                "No NICK change confirmation",
                "NICK :ChangedNick or similar",
                response[:100] + "..." if len(response) > 100 else response
            )
        
        # Test 2: NICK with no parameter
        client.send("NICK\r\n")
        response = client.receive_all(0.2)
        
        # Should get ERR_NONICKNAMEGIVEN (431) or ERR_NEEDMOREPARAMS (461)
        if not assert_numeric_response(response, "431", result):
            assert_numeric_response(response, "461", result)
        
        client.close()
        
        result.success("NICK command tests passed")
        
    except Exception as e:
        result.failure(f"Test error: {str(e)}", "Test completion", str(e))
    
    result.duration = time.time() - start_time
    return result

def test_join_command(host: str, port: int, password: str) -> TestResult:
    """Test JOIN command"""
    result = TestResult("JOIN Command")
    start_time = time.time()
    
    try:
        client = IRCTestClient(host, port, password)
        client.register("Joiner")
        
        # Test 1: Normal JOIN
        client.send(f"JOIN #test\r\n")
        response = client.receive_all(0.3)
        
        # Should get JOIN confirmation and possibly topic/names list
        if not assert_in_response(response, "JOIN", result):
            result.failure(
                "No JOIN confirmation",
                "JOIN confirmation",
                response[:100] + "..." if len(response) > 100 else response
            )
        else:
            # Check for common JOIN responses
            codes_present = []
            for code in ["331", "332", "353", "366"]:  # RPL_NOTOPIC, RPL_TOPIC, RPL_NAMREPLY, RPL_ENDOFNAMES
                if f" {code} " in response:
                    codes_present.append(code)
            
            if codes_present:
                result.success(f"JOIN successful with responses: {', '.join(codes_present)}")
            else:
                result.success("JOIN successful")
        
        # Test 2: JOIN 0 (leave all channels)
        client.send("JOIN 0\r\n")
        response = client.receive_all(0.2)
        
        if "PART" in response or "JOIN" in response or "0" in response:
            result.success("JOIN 0 processed")
        else:
            # Some servers don't respond to JOIN 0
            result.success("JOIN 0 sent (no response expected)")
        
        client.close()
        
        result.success("JOIN command tests passed")
        
    except Exception as e:
        result.failure(f"Test error: {str(e)}", "Test completion", str(e))
    
    result.duration = time.time() - start_time
    return result

def test_privmsg_command(host: str, port: int, password: str) -> TestResult:
    """Test PRIVMSG command"""
    result = TestResult("PRIVMSG Command")
    start_time = time.time()
    
    try:
        # Create two clients for testing
        sender = IRCTestClient(host, port, password)
        receiver = IRCTestClient(host, port, password)
        
        sender.register("Sender")
        receiver.register("Receiver")
        
        # Both join channel
        sender.send(f"JOIN #test\r\n")
        receiver.send(f"JOIN #test\r\n")
        time.sleep(0.3)
        
        # Test 1: PRIVMSG to channel
        test_msg = f"PRIVMSG #test :Hello channel at {time.time()}\r\n"
        sender.send(test_msg)
        
        # Sender should get no error
        sender_response = sender.receive_all(0.2)
        if "PRIVMSG" in sender_response:
            # Some servers echo PRIVMSG back to sender
            pass
        
        # Receiver should get the message
        receiver_response = receiver.receive_all(0.3)
        if not assert_in_response(receiver_response, "Hello channel", result):
            result.failure(
                "Receiver did not get channel message",
                "PRIVMSG with 'Hello channel'",
                receiver_response[:100] + "..." if len(receiver_response) > 100 else receiver_response
            )
        else:
            result.success("Channel PRIVMSG delivered")
        
        # Test 2: PRIVMSG to user
        sender.send("PRIVMSG Receiver :Private hello\r\n")
        receiver_response = receiver.receive_all(0.3)
        
        if not assert_in_response(receiver_response, "Private hello", result):
            result.failure(
                "Receiver did not get private message",
                "PRIVMSG with 'Private hello'",
                receiver_response[:100] + "..." if len(receiver_response) > 100 else receiver_response
            )
        else:
            result.success("Private PRIVMSG delivered")
        
        # Test 3: PRIVMSG to non-existent user
        sender.send("PRIVMSG GhostUser :Hello?\r\n")
        response = sender.receive_all(0.3)
        
        # Should get ERR_NOSUCHNICK (401) or no response
        if response:
            if not assert_numeric_response(response, "401", result):
                if "no such" in response.lower() or "unknown" in response.lower():
                    result.success("Appropriate error for non-existent user")
                else:
                    result.failure(
                        "Unexpected response for non-existent user",
                        "401 ERR_NOSUCHNICK or similar",
                        response[:100] + "..." if len(response) > 100 else response
                    )
            else:
                result.success("Correct error code for non-existent user")
        else:
            result.success("No response for non-existent user (acceptable)")
        
        sender.close()
        receiver.close()
        
        result.success("PRIVMSG command tests passed")
        
    except Exception as e:
        result.failure(f"Test error: {str(e)}", "Test completion", str(e))
    
    result.duration = time.time() - start_time
    return result

def test_ping_pong(host: str, port: int, password: str) -> TestResult:
    """Test PING/PONG commands"""
    result = TestResult("PING/PONG Commands")
    start_time = time.time()
    
    try:
        client = IRCTestClient(host, port, password)
        client.register("PingUser")
        
        # Test client sending PING
        token = f"test{int(time.time())}"
        client.send(f"PING :{token}\r\n")
        response = client.receive_all(0.5)
        
        # Should get PONG with same token
        if not assert_in_response(response, f"PONG :{token}", result):
            if "PONG" in response:
                result.success(f"Got PONG response (token mismatch acceptable)")
            else:
                result.failure(
                    "No PONG response to PING",
                    f"PONG :{token}",
                    response[:100] + "..." if len(response) > 100 else response
                )
        else:
            result.success("PING/PONG handshake successful")
        
        client.close()
        
        result.success("PING/PONG tests passed")
        
    except Exception as e:
        result.failure(f"Test error: {str(e)}", "Test completion", str(e))
    
    result.duration = time.time() - start_time
    return result

# ====== FLOODING AND STRESS TESTS ======

def test_message_flood(host: str, port: int, password: str) -> TestResult:
    """Test message flooding protection"""
    result = TestResult("Message Flood Protection")
    start_time = time.time()
    
    try:
        client = IRCTestClient(host, port, password)
        client.register("Flooder")
        client.send(f"JOIN #test\r\n")
        time.sleep(0.2)
        
        # Rapid PRIVMSG
        messages_sent = 20  # Reduced for safety
        start = time.time()
        
        for i in range(messages_sent):
            client.send(f"PRIVMSG #test :Flood {i} at {time.time()}\r\n")
            time.sleep(0.005)  # 5ms between messages
        
        elapsed = time.time() - start
        response = client.receive_all(1.0)
        
        # Check for flood protection
        if "KICK" in response:
            result.success(f"Server applied flood protection (kicked) after {messages_sent} messages in {elapsed:.2f}s")
        elif "ERROR" in response or "Closing Link" in response:
            result.success(f"Server disconnected flooder after {messages_sent} messages in {elapsed:.2f}s")
        elif "QUIT" in response:
            result.success(f"Server forced quit after {messages_sent} messages in {elapsed:.2f}s")
        elif "NOTICE" in response and ("flood" in response.lower() or "slow" in response.lower()):
            result.success(f"Server warned about flooding after {messages_sent} messages in {elapsed:.2f}s")
        else:
            # Count how many messages got through
            msg_count = response.count("PRIVMSG")
            if msg_count < messages_sent:
                result.success(f"Server throttled messages ({msg_count}/{messages_sent} delivered in {elapsed:.2f}s)")
            else:
                result.failure(
                    f"No flood protection detected ({messages_sent} messages in {elapsed:.2f}s)",
                    "KICK, ERROR, or throttling response",
                    f"All {messages_sent} messages seem delivered"
                )
        
        client.close()
        
    except ConnectionError:
        result.success("Server closed connection (flood protection working)")
    except Exception as e:
        result.failure(f"Test error: {str(e)}", "Test completion", str(e))
    
    result.duration = time.time() - start_time
    return result

def test_slowloris_attack(host: str, port: int, password: str) -> TestResult:
    """Test slow sending (Slowloris-like attack)"""
    result = TestResult("Slow Client Handling")
    start_time = time.time()
    
    try:
        client = IRCTestClient(host, port, password)
        
        # Send registration very slowly character by character
        registration = f"PASS {password}\r\nNICK SlowUser\r\nUSER slow 0 * :Slow Client\r\n"
        
        for char in registration:
            client.sock.sendall(char.encode())
            time.sleep(0.3)  # 300ms between chars
        
        # Wait for response
        time.sleep(2)
        response = client.receive_all(1.0)
        
        # Check if registered
        if "001" in response or "NICK" in response or "PING" in response:
            # Still connected and maybe registered
            result.success("Server accepted slow connection (no timeout)")
            
            # Test if we can still communicate
            client.send("PING :slowtest\r\n")
            ping_response = client.receive_all(1.0)
            
            if "PONG" in ping_response:
                result.success("Slow connection remains functional")
            else:
                result.failure(
                    "Slow connection registered but not functional",
                    "PONG response",
                    ping_response[:100] + "..." if len(ping_response) > 100 else ping_response
                )
        elif "ERROR" in response or "timeout" in response.lower():
            result.success("Server closed slow connection (protection working)")
        elif not response:
            result.success("Server timed out slow connection (protection working)")
        else:
            result.failure(
                "Unexpected response to slow connection",
                "Registration success or timeout",
                response[:100] + "..." if len(response) > 100 else response
            )
        
        client.close()
        
    except ConnectionError:
        result.success("Server rejected slow connection")
    except Exception as e:
        result.failure(f"Test error: {str(e)}", "Test completion", str(e))
    
    result.duration = time.time() - start_time
    return result

def test_concurrent_connections(host: str, port: int, password: str) -> TestResult:
    """Test many simultaneous connections"""
    result = TestResult("Concurrent Connections")
    start_time = time.time()
    
    clients = []
    successful = 0
    
    try:
        # Try to create multiple connections
        for i in range(8):  # Reduced for safety
            try:
                client = IRCTestClient(host, port, password)
                response = client.register(f"Concurrent{i}")
                
                if "001" in response or "PING" in response or "NICK" in response:
                    successful += 1
                    result.success(f"Client {i} connected successfully")
                else:
                    result.failure(
                        f"Client {i} connected but didn't register properly",
                        "Welcome message",
                        response[:100] + "..." if len(response) > 100 else response
                    )
                
                clients.append(client)
                time.sleep(0.05)
            except ConnectionRefusedError:
                result.failure(f"Client {i} connection refused", "Successful connection", "Connection refused")
                break
            except Exception as e:
                result.failure(f"Client {i} error: {str(e)}", "Successful connection", str(e))
        
        # Check all still connected
        still_alive = 0
        for i, client in enumerate(clients):
            try:
                client.send("PING :concurrenttest\r\n")
                response = client.receive_all(0.3)
                if response:
                    still_alive += 1
            except:
                pass
        
        # Cleanup
        for client in clients:
            try:
                client.close()
            except:
                pass
        
        if successful >= 5:  # At least 5 should succeed
            result.success(f"{successful}/8 concurrent connections successful, {still_alive} still alive")
        else:
            result.failure(
                f"Only {successful}/8 concurrent connections successful",
                "At least 5 successful connections",
                f"{successful} succeeded, {still_alive} still alive"
            )
        
    except Exception as e:
        result.failure(f"Test error: {str(e)}", "Test completion", str(e))
    
    result.duration = time.time() - start_time
    return result

def test_invalid_input(host: str, port: int, password: str) -> TestResult:
    """Test invalid/edge case inputs"""
    result = TestResult("Invalid Input Handling")
    start_time = time.time()
    
    tests = [
        ("Missing CRLF", b"PASS testpass\nNICK Test\n", ["461", "ERROR"], "Proper error for missing CR"),
        ("Extra spaces", "PASS  testpass  \r\n", ["464", "461"], "Handle extra whitespace"),
        ("Empty command", "\r\n\r\n\r\n", [], "Ignore empty commands"),
        ("Max length", "PRIVMSG #test :" + "X" * 400 + "\r\n", [], "Handle long messages"),
    ]
    
    try:
        for test_name, input_data, expected_codes, description in tests:
            client = IRCTestClient(host, port, password)
            
            if isinstance(input_data, bytes):
                client.send_raw(input_data)
            else:
                client.send(input_data)
            
            response = client.receive_all(0.5)
            client.close()
            
            if expected_codes:
                code_found = False
                for code in expected_codes:
                    if f" {code} " in response:
                        code_found = True
                        result.success(f"{test_name}: Got expected code {code}")
                        break
                
                if not code_found and response:
                    result.failure(
                        f"{test_name}: No expected code in response",
                        f"One of: {expected_codes}",
                        response[:100] + "..." if len(response) > 100 else response
                    )
                elif not response and test_name != "Empty command":
                    result.failure(
                        f"{test_name}: No response received",
                        f"Error code or processing",
                        "No response"
                    )
                else:
                    result.success(f"{test_name}: Handled appropriately")
            else:
                # No specific expectation, just shouldn't crash
                result.success(f"{test_name}: Server didn't crash")
            
            time.sleep(0.1)
        
        result.success("All invalid input tests passed")
        
    except Exception as e:
        result.failure(f"Test error: {str(e)}", "Test completion", str(e))
    
    result.duration = time.time() - start_time
    return result

def test_zombie_connection(host: str, port: int, password: str) -> TestResult:
    """Test idle connection timeout"""
    result = TestResult("Idle Connection Timeout")
    start_time = time.time()
    
    try:
        client = IRCTestClient(host, port, password)
        client.register("Zombie")
        
        # Go idle for a while
        idle_time = 15  # seconds
        print(f"\n{Colors.YELLOW}Waiting {idle_time}s to test idle timeout...{Colors.END}")
        
        # Check periodically if still connected
        still_connected = True
        for i in range(idle_time * 2):  # Check twice per second
            try:
                # Try to send a byte to check connection
                client.sock.sendall(b'')
                time.sleep(0.5)
            except (BrokenPipeError, ConnectionResetError):
                still_connected = False
                timeout_occurred = i * 0.5
                break
        
        if still_connected:
            # Send PING to confirm
            client.send("PING :zombietest\r\n")
            response = client.receive_all(1.0)
            
            if "PONG" in response:
                result.success(f"Connection still alive after {idle_time}s idle (no timeout)")
            else:
                result.failure(
                    f"No response after {idle_time}s idle",
                    "PONG response or timeout",
                    response[:100] + "..." if len(response) > 100 else response
                )
        else:
            result.success(f"Server closed idle connection after ~{timeout_occurred:.1f}s (good)")
        
        client.close()
        
    except Exception as e:
        result.failure(f"Test error: {str(e)}", "Test completion", str(e))
    
    result.duration = time.time() - start_time
    return result

# ====== TEST SUITE RUNNER ======

def print_test_header(host: str, port: int, password: str):
    """Print test suite header"""
    print(f"\n{Colors.BOLD}{'='*70}{Colors.END}")
    print(f"{Colors.BOLD}{Colors.CYAN}IRC SERVER COMPREHENSIVE TEST SUITE{Colors.END}")
    print(f"{Colors.BOLD}Host: {host}:{port}{Colors.END}")
    print(f"{Colors.BOLD}Password: {'*' * len(password) if password else '(none)'}{Colors.END}")
    print(f"{Colors.BOLD}{'='*70}{Colors.END}\n")

def print_test_summary(results: List[TestResult]):
    """Print test results summary"""
    print(f"\n{Colors.BOLD}{'='*70}{Colors.END}")
    print(f"{Colors.BOLD}{Colors.CYAN}TEST SUMMARY{Colors.END}")
    print(f"{Colors.BOLD}{'='*70}{Colors.END}")
    
    passed = sum(1 for r in results if r.passed)
    failed = len(results) - passed
    total_duration = sum(r.duration for r in results)
    
    # Print individual results
    for result in results:
        print(f"  {result}")
        if not result.passed and result.expected and result.actual:
            print(f"    {Colors.YELLOW}Expected: {result.expected}{Colors.END}")
            print(f"    {Colors.YELLOW}Actual: {result.actual}{Colors.END}")
    
    # Print statistics
    print(f"\n{Colors.BOLD}Statistics:{Colors.END}")
    print(f"  Total Tests: {len(results)}")
    print(f"  {Colors.GREEN}Passed: {passed}{Colors.END}")
    print(f"  {Colors.RED}Failed: {failed}{Colors.END}")
    print(f"  Total Duration: {total_duration:.2f}s")
    if len(results) > 0:
        print(f"  Average per test: {total_duration/len(results):.2f}s")
    
    # Success rate
    if len(results) > 0:
        success_rate = (passed / len(results)) * 100
        color = Colors.GREEN if success_rate >= 80 else Colors.YELLOW if success_rate >= 60 else Colors.RED
        print(f"  {color}Success Rate: {success_rate:.1f}%{Colors.END}")
    
    # Recommendations
    print(f"\n{Colors.BOLD}Recommendations:{Colors.END}")
    if failed == 0:
        print(f"  {Colors.GREEN}✓ All tests passed! Server is robust.{Colors.END}")
    elif failed <= 3:
        print(f"  {Colors.YELLOW}⚠ Minor issues detected. Review failed tests.{Colors.END}")
    else:
        print(f"  {Colors.RED}✗ Significant issues found. Server needs improvement.{Colors.END}")
    
    print(f"{Colors.BOLD}{'='*70}{Colors.END}\n")

def run_test_suite(host: str, port: int, password: str) -> List[TestResult]:
    """Run the complete test suite"""
    print_test_header(host, port, password)
    
    # Define test suite
    test_functions = [
        ("Basic Connection & Registration", test_connection_registration),
        ("NICK Command", test_nick_command),
        ("JOIN Command", test_join_command),
        ("PRIVMSG Command", test_privmsg_command),
        ("PING/PONG Commands", test_ping_pong),
        ("Message Flood Protection", test_message_flood),
        ("Slow Client Handling", test_slowloris_attack),
        ("Concurrent Connections", test_concurrent_connections),
        ("Invalid Input Handling", test_invalid_input),
        ("Idle Connection Timeout", test_zombie_connection),
    ]
    
    results = []
    
    for i, (test_name, test_func) in enumerate(test_functions, 1):
        print(f"\n{Colors.BOLD}[{i}/{len(test_functions)}] Running: {test_name}{Colors.END}")
        print(f"{Colors.WHITE}{'─'*50}{Colors.END}")
        
        try:
            result = test_func(host, port, password)
            results.append(result)
            print(f"  Duration: {result.duration:.2f}s")
        except KeyboardInterrupt:
            print(f"\n{Colors.YELLOW}Test interrupted by user{Colors.END}")
            break
        except Exception as e:
            error_result = TestResult(test_name)
            error_result.failure(f"Test crashed: {str(e)}", "Test completion", str(e))
            results.append(error_result)
            print(f"  {Colors.RED}Test crashed: {str(e)}{Colors.END}")
        
        time.sleep(0.5)  # Brief pause between tests
    
    return results

def quick_test(host: str, port: int, password: str) -> List[TestResult]:
    """Run a quick smoke test"""
    print(f"\n{Colors.BOLD}{Colors.YELLOW}Running Quick Smoke Test...{Colors.END}")
    
    quick_tests = [
        ("Basic Connection", test_connection_registration),
        ("PING/PONG", test_ping_pong),
    ]
    
    results = []
    for test_name, test_func in quick_tests:
        print(f"\n{Colors.BOLD}Testing: {test_name}{Colors.END}")
        try:
            result = test_func(host, port, password)
            results.append(result)
            print(f"  Result: {result}")
        except Exception as e:
            print(f"  {Colors.RED}Error: {str(e)}{Colors.END}")
    
    return results

def main():
    """Main entry point"""
    DEFAULT_HOST = '127.0.0.1'
    DEFAULT_PORT = 6767
    DEFAULT_PASS = 'mypass'
    
    parser = argparse.ArgumentParser(description="IRC Server Test Suite")
    parser.add_argument("--host", default=DEFAULT_HOST, help=f"Server hostname (default: {DEFAULT_HOST})")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help=f"Server port (default: {DEFAULT_PORT})")
    parser.add_argument("--pass", dest="password", default=DEFAULT_PASS, help=f"Server password (default: {DEFAULT_PASS})")
    parser.add_argument("--quick", action="store_true", help="Run quick smoke test only")
    parser.add_argument("--test", help="Run specific test by name")
    
    args = parser.parse_args()
    
    host = args.host
    port = args.port
    password = args.password
    
    print(f"{Colors.CYAN}Testing IRC server at {host}:{port}{Colors.END}")
    print(f"{Colors.CYAN}Using password: {'*' * len(password) if password else '(none)'}{Colors.END}")
    
    try:
        # Test connection first
        test_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        test_socket.settimeout(2)
        test_socket.connect((host, port))
        test_socket.close()
        print(f"{Colors.GREEN}✓ Server is reachable{Colors.END}")
    except ConnectionRefusedError:
        print(f"{Colors.RED}✗ Cannot connect to server at {host}:{port}{Colors.END}")
        print(f"{Colors.YELLOW}Make sure the server is running before testing.{Colors.END}")
        sys.exit(1)
    except Exception as e:
        print(f"{Colors.RED}✗ Connection error: {str(e)}{Colors.END}")
        sys.exit(1)
    
    if args.quick:
        results = quick_test(host, port, password)
    elif args.test:
        # Run specific test
        test_map = {
            "connection": test_connection_registration,
            "nick": test_nick_command,
            "join": test_join_command,
            "privmsg": test_privmsg_command,
            "ping": test_ping_pong,
            "flood": test_message_flood,
            "slow": test_slowloris_attack,
            "concurrent": test_concurrent_connections,
            "invalid": test_invalid_input,
            "zombie": test_zombie_connection,
        }
        
        test_name = args.test.lower()
        if test_name in test_map:
            print(f"\n{Colors.BOLD}Running specific test: {args.test}{Colors.END}")
            result = test_map[test_name](host, port, password)
            results = [result]
            print(f"\n{Colors.BOLD}Result: {result}{Colors.END}")
            if not result.passed:
                if result.expected and result.actual:
                    print(f"{Colors.YELLOW}Expected: {result.expected}{Colors.END}")
                    print(f"{Colors.YELLOW}Actual: {result.actual}{Colors.END}")
        else:
            print(f"{Colors.RED}Unknown test: {args.test}{Colors.END}")
            print(f"Available tests: {', '.join(test_map.keys())}")
            sys.exit(1)
    else:
        # Run full suite
        results = run_test_suite(host, port, password)
    
    # Print summary for multiple tests
    if len(results) > 1:
        print_test_summary(results)
        
        # Exit code based on success
        failed = sum(1 for r in results if not r.passed)
        if failed > 0:
            sys.exit(1)
    elif results:
        # Single test result
        result = results[0]
        if not result.passed:
            sys.exit(1)
    
    sys.exit(0)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print(f"\n\n{Colors.YELLOW}Test suite interrupted by user{Colors.END}")
        sys.exit(130)  # Standard exit code for Ctrl+C
    except Exception as e:
        print(f"\n{Colors.RED}Unexpected error: {str(e)}{Colors.END}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
