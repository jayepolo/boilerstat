import 'package:boilerstat/screens/authenticate/login_viewmodel.dart';
import 'package:boilerstat/screens/home/home.dart';
import 'package:boilerstat/screens/authenticate/login_screen.dart';
import 'package:flutter/material.dart';

class Wrapper extends StatelessWidget {
  const Wrapper({super.key});

  @override
  Widget build(BuildContext context) {
    // return either Home or Authenticate widget
    return LoginScreen(
      viewModel: LoginViewModel(),
    );
  }
}
