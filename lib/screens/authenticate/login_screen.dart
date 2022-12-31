import 'package:boilerstat/screens/authenticate/login_viewmodel.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

class LoginScreen extends StatefulWidget {
  final LoginViewModel viewModel;

  const LoginScreen({required this.viewModel, super.key});

  @override
  LoginScreenState createState() => LoginScreenState();
}

class LoginScreenState extends State<LoginScreen> {
  final _password = TextEditingController();
  final _email = TextEditingController();

  late LoginViewModel viewModel;

  @override
  void initState() {
    viewModel = widget.viewModel;
    viewModel.fetchData();
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: SafeArea(
          child: Column(
        children: [
          Padding(
            padding: EdgeInsets.all(20),
            child: _buildStatus(),
          ),
          _buildEmail(),
          _buildPaswordField(),
          _buildButton(() {
            viewModel.submit();
          }),
          Expanded(child: _buildDataList())
        ],
      )),
    );
  }

  Widget _buildDataList() {
    return Padding(
      padding: EdgeInsets.all(10),
      child: StreamBuilder<String>(
        builder: (context, snapshot) {
          if (snapshot.data?.isEmpty == true) {
            return Text('Empty data');
          }
          return Text('${snapshot.data!}');
        },
        initialData: '',
        stream: viewModel.dataList,
      ),
    );
  }

  Widget _buildStatus() {
    return StreamBuilder<User>(
      builder: (context, snapshot) {
        String msg;

        return Text(
          snapshot.hasData ? '${snapshot.data?.email}' : 'Not login ',
          maxLines: 1,
          style: TextStyle(fontSize: 20),
        );
      },
      stream: viewModel.login,
    );
  }

  Widget _buildPaswordField() {
    return StreamBuilder(builder: (context, snapshot) {
      return TextField(
        controller: _password,
        onChanged: viewModel.onPassword,
        obscureText: true,
        // keyboardType: TextInputType.,
      );
    });
  }

  Widget _buildEmail() {
    return StreamBuilder(builder: (context, snapshot) {
      return TextField(
        controller: _email,
        onChanged: viewModel.onEmailChanged,
        keyboardType: TextInputType.text,
      );
    });
  }

  Widget _buildButton(Function() onSubmitPressed) {
    return Padding(
      padding: const EdgeInsets.all(10),
      child: TextButton(onPressed: onSubmitPressed, child: const Text('Login')),
    );
  }

  @override
  void dispose() {
    viewModel.dispose();
    super.dispose();
  }
}
