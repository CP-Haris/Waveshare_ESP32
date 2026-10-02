import React, { useEffect, useRef } from 'react';
import { TouchableOpacity, Text, StyleSheet, View, Alert, Animated } from 'react-native';

export type OperationalState = 
  | 'OS_DISABLED'     // 0xFFFF0000
  | 'OS_OFF'          // 0x00000
  | 'OS_WAKEUP'       // 0x10000
  | 'OS_READY_TO_START' // 0x20000
  | 'OS_STARTING'     // 0x30000
  | 'OS_STOPPING'     // 0x40000
  | 'OS_ON';          // 0x50000

interface DisplayTextButtonProps {
  title: string;
  value?: string;
  operationalState: OperationalState;
  onPress: () => void;
  failureReason?: string;
  hasFailure?: boolean;
  failureCount?: number;
  isCharging?: boolean;
  acOutState?: string;
}

const DisplayTextButton: React.FC<DisplayTextButtonProps> = ({ 
  title, 
  value, 
  operationalState, 
  onPress,
  failureReason,
  hasFailure = false,
  failureCount = 0,
  isCharging = false,
  acOutState = ""
}) => {
  const pulseAnim = useRef(new Animated.Value(1)).current;
  const blinkAnim = useRef(new Animated.Value(1)).current;

  // Animation for transitional states (but not for failure states)
  useEffect(() => {
    if (hasFailure) return; // No animation for failure states
    
    const isTransitional = ['OS_WAKEUP', 'OS_READY_TO_START', 'OS_STARTING', 'OS_STOPPING'].includes(operationalState);
    
    // Custom blinking logic for 230V when charging
    const shouldBlink = isCharging && acOutState === "Off" && title === "230 V";
    
    if (shouldBlink) {
      // Start blinking animation (on/off effect)
      const blinkAnimation = Animated.loop(
        Animated.sequence([
          Animated.timing(blinkAnim, {
            toValue: 0,
            duration: 500,
            useNativeDriver: false,
          }),
          Animated.timing(blinkAnim, {
            toValue: 1,
            duration: 500,
            useNativeDriver: false,
          }),
        ])
      );

      blinkAnimation.start();

      return () => {
        blinkAnimation.stop();
      };
    } else if (isTransitional) {
      // Start alpha animation (pulsing opacity) for other transitional states
      const alphaAnimation = Animated.loop(
        Animated.sequence([
          Animated.timing(pulseAnim, {
            toValue: 0.3,
            duration: 800,
            useNativeDriver: true,
          }),
          Animated.timing(pulseAnim, {
            toValue: 1,
            duration: 800,
            useNativeDriver: true,
          }),
        ])
      );

      alphaAnimation.start();

      return () => {
        alphaAnimation.stop();
      };
    } else {
      // Reset animations for stable states
      pulseAnim.setValue(1);
      blinkAnim.setValue(1);
    }
  }, [operationalState, pulseAnim, blinkAnim, hasFailure, isCharging, acOutState, title]);

  const handlePress = () => {
    if (hasFailure) {
      Alert.alert(
        'Component Failure',
        failureReason || 'This component has active failures and cannot be operated.',
        [{ text: 'OK' }]
      );
      return;
    }
    
    if (operationalState === 'OS_DISABLED') {
      Alert.alert(
        'Function Disabled',
        failureReason || 'This function is currently disabled and cannot be activated.',
        [{ text: 'OK' }]
      );
      return;
    }
    
    // Don't allow interaction during transitional states
    if (['OS_WAKEUP', 'OS_READY_TO_START', 'OS_STARTING', 'OS_STOPPING'].includes(operationalState)) {
      return;
    }
    
    onPress();
  };

  const getButtonStyle = () => {
    // Failure state overrides all other states
    if (hasFailure) {
      return [styles.button, styles.buttonFailure];
    }
    
    const baseStyle = [styles.button];
    
    // Special case: 230V charging with AC OUT off - button should be gray (off) while green light blinks
    if (isCharging && acOutState === "Off" && title === "230 V") {
      return [...baseStyle, styles.buttonOff];
    }
    
    switch (operationalState) {
      case 'OS_ON':
        return [...baseStyle, styles.buttonOn];
      case 'OS_OFF':
        return [...baseStyle, styles.buttonOff];
      case 'OS_READY_TO_START':
      case 'OS_WAKEUP':
      case 'OS_STARTING':
      case 'OS_STOPPING':
        return [...baseStyle, styles.buttonOn]; // Use same blue color for all transitional states
      case 'OS_DISABLED':
        return [...baseStyle, styles.buttonDisabled];
      default:
        return [...baseStyle, styles.buttonOff];
    }
  };

  const getCircleStyle = () => {
    // Failure state overrides all other states
    if (hasFailure) {
      return [styles.stateCircle, styles.stateCircleFailure];
    }
    
    const baseStyle = [styles.stateCircle];
    
    // Custom logic for 230V charging states
    if (isCharging && title === "230 V") {
      if (acOutState === "Ready to Start") {
        // Charging + Ready to Start = Constant green light (no blinking)
        return [...baseStyle, styles.stateCircleOn];
      } else if (acOutState === "Off") {
        // Charging + Off = Blinking green light (handled by animation)
        // Return base style - the blinking will be handled by animated backgroundColor
        return [...baseStyle];
      }
    }
    
    switch (operationalState) {
      case 'OS_ON':
        return [...baseStyle, styles.stateCircleOn];
      case 'OS_OFF':
        return [...baseStyle, styles.stateCircleOff];
      case 'OS_READY_TO_START':
        return [...baseStyle, styles.stateCircleReadyToStart];
      case 'OS_WAKEUP':
        return [...baseStyle, styles.stateCircleWakeup];
      case 'OS_STARTING':
        return [...baseStyle, styles.stateCircleStarting];
      case 'OS_STOPPING':
        return [...baseStyle, styles.stateCircleStopping];
      case 'OS_DISABLED':
        return [...baseStyle, styles.stateCircleDisabled];
      default:
        return [...baseStyle, styles.stateCircleOff];
    }
  };

  const isInteractive = !hasFailure && !['OS_DISABLED', 'OS_WAKEUP', 'OS_READY_TO_START', 'OS_STARTING', 'OS_STOPPING'].includes(operationalState);
  const isTransitional = !hasFailure && ['OS_WAKEUP', 'OS_READY_TO_START', 'OS_STARTING', 'OS_STOPPING'].includes(operationalState);
  const is230VCharging = isCharging && acOutState === "Off" && title === "230 V";

  // Get the animated backgroundColor for blinking circles
  const getAnimatedCircleStyle = () => {
    if (is230VCharging) {
      return {
        backgroundColor: blinkAnim.interpolate({
          inputRange: [0, 1],
          outputRange: ['#666666', '#84cc16'], // Blink between gray and green
        }),
      };
    }
    return {};
  };

  return (
    <View style={styles.container}>
      <View style={styles.buttonContainer}>
        <Animated.View
          style={[
            { opacity: isTransitional ? pulseAnim : 1 }
          ]}
        >
          <TouchableOpacity 
            style={getButtonStyle()} 
            onPress={handlePress}
            activeOpacity={isInteractive ? 0.7 : 1}
          >
            <Animated.View 
              style={[
                getCircleStyle(),
                is230VCharging && getAnimatedCircleStyle()
              ]}
            />
            <Text style={[styles.buttonText, (!isInteractive || hasFailure) && styles.buttonTextDisabled]}>
              {title}
            </Text>
          </TouchableOpacity>
        </Animated.View>
      </View>
      <Text style={[styles.buttonValueText, (!isInteractive || hasFailure) && styles.buttonValueTextDisabled]}>
        {value}
      </Text>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    gap: 2,
    flexDirection: 'column',
    alignItems: 'center',
  },
  buttonContainer: {
    // Container for glow effect
  },
  button: {
    position: 'relative',
    width: 80,
    height: 80,
    padding: 8,
    borderRadius: 40,
    justifyContent: 'center',
    alignItems: 'center',
    shadowColor: '#000',
    shadowOffset: {
      width: 0,
      height: 2,
    },
    shadowOpacity: 0.05,
    shadowRadius: 3.84,
    elevation: 0,
  },
  // Stable states
  buttonOn: {
    backgroundColor: '#0075c1', // Blue - fully active
  },
  buttonOff: {
    backgroundColor: '#404040', // Dark gray - off but available
    borderWidth: 1,
    borderColor: '#666666',
  },
  // Transitional states
  buttonReadyToStart: {
    backgroundColor: '#2ecc71', // Green - ready to start
    borderWidth: 1,
    borderColor: '#58d68d',
  },
  buttonWakeup: {
    backgroundColor: '#4a90e2', // Light blue - preparing
    borderWidth: 1,
    borderColor: '#6bb6ff',
  },
  buttonStarting: {
    backgroundColor: '#f39c12', // Orange - starting up
    borderWidth: 1,
    borderColor: '#ffb84d',
  },
  buttonStopping: {
    backgroundColor: '#e67e22', // Dark orange - shutting down
    borderWidth: 1,
    borderColor: '#ff8c42',
  },
  // Disabled state
  buttonDisabled: {
    backgroundColor: '#8B0000', // Dark red - disabled
    borderWidth: 1,
    borderColor: '#ff4444',
    opacity: 0.6,
  },
  stateCircle: {
    position: 'absolute',
    top: 0,
    marginTop: -6,
    left: '50%',
    width: 12,
    height: 12,
    padding: 4,
    borderRadius: 12,
    justifyContent: 'center',
    alignItems: 'center',
    borderWidth: 4,
    borderColor: '#202020'
  },
  // Stable state circles
  stateCircleOn: {
    backgroundColor: '#84cc16', // Green - active
  },
  stateCircleOff: {
    backgroundColor: '#666666', // Gray - off
  },
  // Transitional state circles
  stateCircleReadyToStart: {
    backgroundColor: '#58d68d', // Green - ready to start
  },
  stateCircleWakeup: {
    backgroundColor: '#6bb6ff', // Light blue
  },
  stateCircleStarting: {
    backgroundColor: '#ffb84d', // Light orange
  },
  stateCircleStopping: {
    backgroundColor: '#ff8c42', // Orange
  },
  stateCircleDisabled: {
    backgroundColor: '#ff4444', // Red - disabled
  },
  // Failure state circle
  stateCircleFailure: {
    backgroundColor: '#DC143C', // Crimson red - component failure
    borderColor: '#FF6B6B',
    borderWidth: 6,
  },
  buttonText: {
    color: '#ffffff',
    fontSize: 20,
    fontWeight: '600',
  },
  buttonTextDisabled: {
    color: '#cccccc',
  },
  buttonValueText: {
    color: '#ffffff',
    fontSize: 12,
    fontWeight: '600',
  },
  buttonValueTextDisabled: {
    color: '#999999',
  },
  // Failure state - overrides all others
  buttonFailure: {
    backgroundColor: '#DC143C', // Crimson red - component failure
    borderWidth: 2,
    borderColor: '#FF6B6B',
    shadowColor: '#DC143C',
    shadowOffset: {
      width: 0,
      height: 0,
    },
    shadowOpacity: 0.8,
    shadowRadius: 8,
    elevation: 8,
  },
});

export default DisplayTextButton;
